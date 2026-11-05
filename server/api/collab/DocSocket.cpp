#include "collab/DocSocket.h"

#include <drogon/utils/coroutine.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <variant>

#include "collab/LoadDrawing.h"
#include "collab/Messages.h"
#include "collab/OpPipeline.h"
#include "collab/Presence.h"
#include "collab/RemoteOps.h"
#include "collab/Snapshots.h"
#include "db/DocRepo.h"
#include "db/UserRepo.h"
#include "limits/TokenBucket.h"
#include "logging/Log.h"
#include "ops/Json.h"

using drogon::CloseCode;
using drogon::WebSocketConnectionPtr;
using nlohmann::json;

namespace {

// Application close codes live in 4000-4999; these echo the HTTP statuses they stand for.
constexpr auto closeUnauthorized = static_cast<CloseCode>(4401);
constexpr auto closeNotFound = static_cast<CloseCode>(4404);
constexpr auto closeLoadFailed = CloseCode::kUnexpectedCondition;  // 1011, a server-side failure

constexpr std::string_view pathPrefix = "/ws/docs/";

// Cursor moves beyond this are dropped: plenty for smooth cursors, without flooding Redis.
constexpr double cursorsPerSecond = 10;
// The presence hash outlives its last refresh by this long, so a document nobody visits again is cleaned up.
constexpr int presenceKeyLifetimeSeconds = 120;
// Published on a document's channel, beside its ops, when its presence changes.
constexpr const char* presenceNotice = R"({"presence":true})";

// Each connection may send this many ops a second on average, in bursts of up to opBurst: well above what
// a person dragging shapes produces, while one connection can't monopolise its room's queue.
constexpr double opsPerSecond = 30;
constexpr double opBurst = 60;

// A connection that sends nothing for this long is closed; clients ping well within it.
constexpr std::chrono::seconds silenceLimit{45};
constexpr double heartbeatCheckSeconds = 5;
constexpr auto closeIdle = static_cast<CloseCode>(4408);

int64_t steadyNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::atomic<vec::SessionId> nextSessionId = 1;

void sendJson(const WebSocketConnectionPtr& connection, const json& message) {
    connection->send(message.dump());
}

}  // namespace

// One connection's identity and room. Created on the connection's thread, joined to a room on the room's,
// and closed on the connection's again, so the fields shared between threads are atomic or locked.
struct DocSession {
    vec::SessionId id = 0;
    std::string userId;
    std::string documentId;
    vec::Role role = vec::Role::Viewer;  // written before `ready` is set, read only after
    std::atomic<bool> ready = false;     // a member of `room`
    std::atomic<bool> closed = false;
    std::atomic<int64_t> lastHeardMs = 0;  // steady clock; when the client last sent anything
    trantor::TimerId heartbeatTimer = 0;   // set and cancelled on the connection's own thread
    // Used only on the connection's own thread, where its messages arrive one at a time.
    vec::TokenBucket opRate{opsPerSecond, opBurst};
    vec::TokenBucket cursorRate{cursorsPerSecond, cursorsPerSecond};
    std::string name;  // the user's email, written before `ready` is set

    std::optional<Point> cursor() {
        const std::lock_guard lock(mutex);
        return cursor_;
    }
    void setCursor(Point cursor) {
        const std::lock_guard lock(mutex);
        cursor_ = cursor;
    }

    std::shared_ptr<vec::Room> room() {
        const std::lock_guard lock(mutex);
        return room_;
    }
    void setRoom(std::shared_ptr<vec::Room> room) {
        const std::lock_guard lock(mutex);
        room_ = std::move(room);
    }

private:
    std::mutex mutex;
    std::shared_ptr<vec::Room> room_;
    std::optional<Point> cursor_;
};

void DocSocket::handleNewConnection(const drogon::HttpRequestPtr& request,
                                    const WebSocketConnectionPtr& connection) {
    std::string token = request->getParameter("token");
    if (token.empty())
        token = std::string(vec::bearerToken(request->getHeader("authorization")).value_or(""));
    const auto userId = tokens_.verify(token);
    if (!userId) return connection->shutdown(closeUnauthorized, "unauthorized");

    auto session = std::make_shared<DocSession>();
    session->id = nextSessionId++;
    session->userId = *userId;
    session->documentId = request->path().substr(pathPrefix.size());
    connection->setContext(session);

    // Checks for silence from the first moment, so a client that never even joins is closed too.
    session->lastHeardMs = steadyNowMs();
    session->heartbeatTimer =
        drogon::app().getLoop()->runEvery(heartbeatCheckSeconds, [weak = std::weak_ptr(connection), session] {
            const auto open = weak.lock();
            if (!open ||
                steadyNowMs() - session->lastHeardMs < std::chrono::milliseconds(silenceLimit).count()) {
                return;
            }
            open->shutdown(closeIdle, "idle");
        });

    drogon::async_run([this, connection, session]() -> drogon::Task<> {
        const auto role = co_await vec::MemberRepo(database_).roleOf(session->documentId, session->userId);
        if (!role) {
            connection->shutdown(closeNotFound, "not_found");
            co_return;
        }
        session->role = *role;
        const auto user = co_await vec::UserRepo(database_).findById(session->userId);
        session->name = user ? user->email : "";
        joinRoom(connection, session);
    });
}

void DocSocket::joinRoom(const WebSocketConnectionPtr& connection,
                         const std::shared_ptr<DocSession>& session) {
    const auto room = rooms_->roomFor(session->documentId);
    room->loop()->queueInLoop([this, connection, session, room] {
        // A weak pointer, so that the room doesn't keep a closed connection alive.
        const auto result = room->join(
            session->id, {session->userId, [weak = std::weak_ptr(connection)](const std::string& message) {
                              if (const auto open = weak.lock()) open->send(message);
                          }});
        if (result == vec::Room::JoinResult::Unloaded) return joinRoom(connection, session);  // a fresh room

        session->setRoom(room);
        session->ready = true;
        // If the connection closed while joining, its close handler saw `ready` false and left this to us.
        if (session->closed) return leaveRoom(session);

        vec::logInfo("joined room", {},
                     {{"document", session->documentId}, {"members", room->memberCount()}});
        // Queued first, so that every join message waits for the drawing.
        if (result == vec::Room::JoinResult::JoinedAndMustLoad) loadRoom(room);
        sendJson(connection, {{"type", "ready"}, {"role", vec::toString(session->role)}});
    });
}

void DocSocket::loadRoom(const std::shared_ptr<vec::Room>& room) {
    room->enqueue([this, room](std::function<void()> done) {
        drogon::async_run([this, room, done]() -> drogon::Task<> {
            const auto started = std::chrono::steady_clock::now();
            std::shared_ptr<vec::LoadedDrawing> loaded;
            std::string error;
            try {
                auto result = co_await vec::loadDrawing(database_, room->documentId());
                if (result)
                    loaded = std::make_shared<vec::LoadedDrawing>(std::move(*result));
                else
                    error = result.error();
            } catch (const std::exception& e) {
                error = e.what();
            }
            const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(
                                    std::chrono::steady_clock::now() - started)
                                    .count();

            room->loop()->queueInLoop([this, room, loaded, error, millis, done] {
                if (loaded) {
                    room->finishLoad(std::move(loaded->diagram), loaded->lastSeq, loaded->snapshotSeq);
                    vec::logInfo("loaded room", {},
                                 {{"document", room->documentId()},
                                  {"lastSeq", room->lastSeq()},
                                  {"shapes", room->diagram().getObjectCount()},
                                  {"ms", millis}});
                    subscribe(room->documentId());
                    // A long tail to replay means the next load would be slow too: snapshot it now.
                    vec::snapshotIfDue(database_, room);
                } else {
                    room->failLoad();
                    vec::logError("room load failed", {},
                                  {{"document", room->documentId()}, {"error", error}});
                }
                done();
            });
        });
    });
}

void DocSocket::handleJoin(const WebSocketConnectionPtr& connection,
                           const std::shared_ptr<DocSession>& session, int64_t lastSeq) {
    const auto room = session->room();
    room->loop()->queueInLoop([this, connection, session, room, lastSeq] {
        // A room task, so no op is processed between building the reply and marking the member joined.
        const bool queued =
            room->enqueue([this, connection, session, room, lastSeq](std::function<void()> done) {
                // Runs after the load, which is queued ahead of every join.
                if (room->loadState() != vec::Room::LoadState::Loaded) {
                    sendJson(connection, {{"type", "error"},
                                          {"code", "load_failed"},
                                          {"message", "the document can't be opened"}});
                    connection->shutdown(closeLoadFailed, "load_failed");
                    return done();
                }

                json reply = {{"type", "joined"}, {"headSeq", room->lastSeq()}};
                if (const auto missed = room->opsSince(lastSeq)) {
                    json ops = json::array();
                    for (const vec::RecentOp& recent : *missed) {
                        ops.push_back({{"seq", recent.seq}, {"userId", recent.userId}, {"op", recent.op}});
                    }
                    reply["ops"] = std::move(ops);
                } else {
                    // Too far behind, or claiming to be ahead: start over from the whole drawing.
                    reply["snapshot"] = vec::diagramToJson(room->diagram());
                }
                sendJson(connection, reply);
                room->markJoined(session->id);
                writePresence(session, true);
                done();
            });
        if (!queued)
            sendJson(connection, {{"type", "error"}, {"code", "busy"}, {"message", "try again shortly"}});
    });
}

void DocSocket::subscribe(const std::string& documentId) {
    const std::lock_guard lock(subscriptionsMutex_);
    if (!subscriptions_.insert(documentId).second) return;  // already listening, for an earlier room
    subscriber_->subscribe("doc:" + documentId,
                           [this, documentId](const std::string&, const std::string& message) {
                               // Looked up per message rather than captured: the document's room may have
                               // been replaced since.
                               const auto room = rooms_->find(documentId);
                               if (!room) return;
                               if (message == presenceNotice) return sendPresence(room);
                               room->loop()->queueInLoop([this, room, message] {
                                   // A room task, so that it can't interleave with an op this instance is
                                   // committing. If the queue is full the op is dropped here, and caught up
                                   // from Postgres before the room's next op.
                                   room->enqueue([this, room, message](std::function<void()> done) {
                                       drogon::async_run([this, room, message, done]() -> drogon::Task<> {
                                           co_await vec::applyRemoteOp(database_, room, message, instanceId_);
                                           done();
                                       });
                                   });
                               });
                           });
}

namespace {

std::string presenceKey(const std::string& documentId) { return "presence:" + documentId; }

void logRedisError(const char* what, const std::string& documentId, const std::exception& e) {
    vec::logError(what, {}, {{"document", documentId}, {"error", e.what()}});
}

}  // namespace

void DocSocket::writePresence(const std::shared_ptr<DocSession>& session, bool announce) {
    if (!session->ready) return;  // not listed until joined
    const std::string field = instanceId_ + ":" + std::to_string(session->id);
    const std::string entry =
        vec::encodePresence({session->userId, session->name, session->cursor(), vec::unixMillis()});
    const std::string documentId = session->documentId;
    redis_->execCommandAsync(
        [redis = redis_, documentId, announce](const drogon::nosql::RedisResult&) {
            // After the write, so that whoever reads on the notice sees it.
            redis->execCommandAsync([](const drogon::nosql::RedisResult&) {},
                                    [documentId](const std::exception& e) {
                                        logRedisError("presence expiry failed", documentId, e);
                                    },
                                    "EXPIRE %s %d", presenceKey(documentId).c_str(),
                                    presenceKeyLifetimeSeconds);
            if (!announce) return;
            redis->execCommandAsync([](const drogon::nosql::RedisResult&) {},
                                    [documentId](const std::exception& e) {
                                        logRedisError("presence notice failed", documentId, e);
                                    },
                                    "PUBLISH %s %s", ("doc:" + documentId).c_str(), presenceNotice);
        },
        [documentId](const std::exception& e) { logRedisError("presence write failed", documentId, e); },
        "HSET %s %s %s", presenceKey(documentId).c_str(), field.c_str(), entry.c_str());
}

void DocSocket::removePresence(const std::shared_ptr<DocSession>& session) {
    const std::string field = instanceId_ + ":" + std::to_string(session->id);
    const std::string documentId = session->documentId;
    redis_->execCommandAsync(
        [redis = redis_, documentId](const drogon::nosql::RedisResult&) {
            redis->execCommandAsync([](const drogon::nosql::RedisResult&) {},
                                    [documentId](const std::exception& e) {
                                        logRedisError("presence notice failed", documentId, e);
                                    },
                                    "PUBLISH %s %s", ("doc:" + documentId).c_str(), presenceNotice);
        },
        [documentId](const std::exception& e) { logRedisError("presence removal failed", documentId, e); },
        "HDEL %s %s", presenceKey(documentId).c_str(), field.c_str());
}

void DocSocket::sendPresence(const std::shared_ptr<vec::Room>& room) {
    redis_->execCommandAsync(
        [room](const drogon::nosql::RedisResult& result) {
            std::vector<std::string> values;
            const auto items = result.asArray();
            for (size_t i = 1; i < items.size(); i += 2)
                values.push_back(items[i].asString());  // field, value, ...
            const std::string message = vec::presenceMessage(values, vec::unixMillis());
            room->loop()->queueInLoop([room, message] { room->broadcast(message); });
        },
        [documentId = room->documentId()](const std::exception& e) {
            logRedisError("presence read failed", documentId, e);
        },
        "HGETALL %s", presenceKey(room->documentId()).c_str());
}

void DocSocket::unsubscribeIfUnused(const std::string& documentId) {
    const std::lock_guard lock(subscriptionsMutex_);
    // A room loaded again since this one unloaded still needs the channel.
    if (rooms_->find(documentId) || subscriptions_.erase(documentId) == 0) return;
    subscriber_->unsubscribe("doc:" + documentId);
}

void DocSocket::leaveRoom(const std::shared_ptr<DocSession>& session) {
    removePresence(session);
    const auto room = session->room();
    room->loop()->queueInLoop([this, session, room] {
        const auto emptied = room->leave(session->id);
        vec::logInfo("left room", {}, {{"document", session->documentId}, {"members", room->memberCount()}});
        if (!emptied) return;

        room->loop()->runAfter(unloadDelay_.count(), [this, room, token = *emptied] {
            if (!room->unloadIfStillEmpty(token)) return;
            rooms_->remove(room);
            unsubscribeIfUnused(room->documentId());
            vec::logInfo("unloaded room", {}, {{"document", room->documentId()}});
        });
    });
}

void DocSocket::handleOp(const WebSocketConnectionPtr& connection, const std::shared_ptr<DocSession>& session,
                         const vec::OpMessage& op) {
    const auto reject = [&](const char* reason, const char* message) {
        sendJson(
            connection,
            {{"type", "reject"}, {"clientOpId", op.clientOpId}, {"reason", reason}, {"message", message}});
    };
    if (session->role < vec::Role::Editor) return reject("forbidden", "viewers can't edit");
    if (!session->opRate.tryTake()) return reject("rate_limited", "too many edits; slow down");
    const auto room = session->room();
    room->loop()->queueInLoop([this, room, op, connection, session] {
        vec::OpSender sender{session->id, session->userId,
                             [weak = std::weak_ptr(connection)](const std::string& message) {
                                 if (const auto open = weak.lock()) open->send(message);
                             }};
        const bool queued =
            room->enqueue([this, room, op, sender,
                           queuedAt = std::chrono::steady_clock::now()](std::function<void()> done) {
                drogon::async_run([this, room, op, sender, queuedAt, done]() -> drogon::Task<> {
                    co_await vec::processOp(database_, room, sender, op, queuedAt, publish_);
                    done();
                });
            });
        if (!queued) {
            sender.send(json{{"type", "reject"},
                             {"clientOpId", op.clientOpId},
                             {"reason", "busy"},
                             {"message", "too many edits waiting; try again shortly"}}
                            .dump());
        }
    });
}

void DocSocket::handleNewMessage(const WebSocketConnectionPtr& connection, std::string&& message,
                                 const drogon::WebSocketMessageType& type) {
    const auto session = connection->getContext<DocSession>();
    if (session) session->lastHeardMs = steadyNowMs();       // any frame counts, protocol pings included
    if (type != drogon::WebSocketMessageType::Text) return;  // protocol pings are answered by Drogon
    if (!session || !session->ready) {
        return sendJson(connection, {{"type", "error"}, {"code", "not_ready"}});
    }
    const auto parsed = vec::parseClientMessage(message);
    if (!parsed && parsed.error().code == "bad_op") {
        return sendJson(connection, {{"type", "reject"},
                                     {"clientOpId", parsed.error().clientOpId},
                                     {"reason", "bad_op"},
                                     {"message", parsed.error().message}});
    }
    if (!parsed) {
        return sendJson(
            connection,
            {{"type", "error"}, {"code", parsed.error().code}, {"message", parsed.error().message}});
    }
    if (std::holds_alternative<vec::PingMessage>(*parsed)) {
        writePresence(session, false);  // still here: keeps the entry from going stale
        return sendJson(connection, {{"type", "pong"}});
    }
    if (const auto* cursor = std::get_if<vec::CursorMessage>(&*parsed)) {
        if (!session->cursorRate.tryTake()) return;  // moving fast: the next move will carry the position
        session->setCursor({cursor->x, cursor->y});
        return writePresence(session, true);
    }
    if (const auto* op = std::get_if<vec::OpMessage>(&*parsed)) return handleOp(connection, session, *op);
    handleJoin(connection, session, std::get<vec::JoinMessage>(*parsed).lastSeq);
}

void DocSocket::handleConnectionClosed(const WebSocketConnectionPtr& connection) {
    const auto session = connection->getContext<DocSession>();
    if (!session) return;
    drogon::app().getLoop()->invalidateTimer(session->heartbeatTimer);
    session->closed = true;
    // Not joined yet: the join will see `closed` and leave by itself.
    if (session->ready) leaveRoom(session);
}
