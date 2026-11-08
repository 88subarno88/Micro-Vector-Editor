#include "DocClient.h"

#include <drogon/HttpRequest.h>
#include <drogon/utils/Utilities.h>

#include <utility>

#include "ops/Apply.h"
#include "ops/Json.h"

using nlohmann::json;

namespace {

constexpr std::chrono::seconds joinTimeout{10};

}  // namespace

DocClient::DocClient(uint16_t port, const std::string& documentId, const std::string& token) {
    socket_ = drogon::WebSocketClient::newWebSocketClient("127.0.0.1", port);
    socket_->setMessageHandler([this](std::string&& message, const drogon::WebSocketClientPtr&,
                                      const drogon::WebSocketMessageType& type) {
        if (type == drogon::WebSocketMessageType::Text) handle(message);
    });
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setPath("/ws/docs/" + documentId);
    request->setParameter("token", token);
    socket_->connectToServer(
        request, [](drogon::ReqResult, const drogon::HttpResponsePtr&, const drogon::WebSocketClientPtr&) {});

    std::unique_lock lock(mutex_);
    changed_.wait_for(lock, joinTimeout, [this] { return joined_ || !problem_.empty(); });
}

DocClient::~DocClient() { socket_->stop(); }

void DocClient::send(const vec::Operation& op) {
    std::lock_guard lock(mutex_);
    // Unique for the document, as the protocol requires, even across clients.
    const std::string clientOpId = drogon::utils::getUuid();
    pending_.emplace(clientOpId, op);
    socket_->getConnection()->send(
        json{{"type", "op"}, {"clientOpId", clientOpId}, {"baseSeq", lastSeq_}, {"op", op}}.dump());
}

bool DocClient::waitUntilSettled(int64_t seq, std::chrono::seconds timeout) {
    std::unique_lock lock(mutex_);
    return changed_.wait_for(lock, timeout, [&] { return pending_.empty() && lastSeq_ >= seq; });
}

json DocClient::replica() {
    std::lock_guard lock(mutex_);
    return vec::diagramToJson(diagram_);
}

int64_t DocClient::lastSeq() {
    std::lock_guard lock(mutex_);
    return lastSeq_;
}

int DocClient::acked() {
    std::lock_guard lock(mutex_);
    return acked_;
}

int DocClient::rejected() {
    std::lock_guard lock(mutex_);
    return rejected_;
}

std::string DocClient::problem() {
    std::lock_guard lock(mutex_);
    return problem_;
}

// Runs on the client's event loop.
void DocClient::handle(const std::string& message) {
    const json m = json::parse(message, nullptr, false);
    const std::string type = m.is_object() ? m.value("type", "") : "";
    std::lock_guard lock(mutex_);
    const auto confirm = [&](int64_t seq, const json& op) {
        auto decoded = vec::decodeOperation(op);
        if (decoded) {
            confirmed_.emplace(seq, std::move(*decoded));
        } else {
            problem_ += "undecodable op " + op.dump() + "\n";
        }
    };

    if (type == "ready") {
        socket_->getConnection()->send(json{{"type", "join"}, {"lastSeq", 0}}.dump());
    } else if (type == "joined") {
        if (m.contains("snapshot")) {
            auto snapshot = vec::decodeDiagram(m["snapshot"]);
            if (!snapshot) problem_ += "undecodable snapshot\n";
            if (snapshot) diagram_ = std::move(*snapshot);
            lastSeq_ = m["headSeq"];
        } else {
            for (const json& op : m["ops"]) confirm(op["seq"], op["op"]);
        }
        joined_ = true;
    } else if (type == "op") {
        confirm(m["seq"], m["op"]);
    } else if (type == "ack") {
        const auto sent = pending_.find(m["clientOpId"]);
        if (sent == pending_.end()) {
            problem_ += "ack for an unknown op: " + message + "\n";
        } else {
            confirmed_.emplace(m["seq"].get<int64_t>(), std::move(sent->second));
            pending_.erase(sent);
            ++acked_;
        }
    } else if (type == "reject") {
        pending_.erase(m.value("clientOpId", ""));
        ++rejected_;
    } else if (type != "presence" && type != "pong") {
        problem_ += "unexpected message: " + message + "\n";
    }
    applyReady();
    changed_.notify_all();
}

void DocClient::applyReady() {
    for (auto next = confirmed_.find(lastSeq_ + 1); next != confirmed_.end();
         next = confirmed_.find(lastSeq_ + 1)) {
        if (!vec::apply(diagram_, next->second)) {
            problem_ += "confirmed op " + std::to_string(next->first) + " didn't apply\n";
        }
        lastSeq_ = next->first;
        confirmed_.erase(next);
    }
}
