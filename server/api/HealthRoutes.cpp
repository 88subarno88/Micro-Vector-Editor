#include "HealthRoutes.h"

#include <drogon/drogon.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "RequestLogging.h"
#include "logging/Log.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;

namespace {

using Respond = std::function<void(const HttpResponsePtr&)>;

// Drogon queues a query until it has a connection, so a dead service never calls back.
constexpr double readinessTimeoutSeconds = 2.0;

// One /readyz request. It responds once both checks have answered, or at the timeout, whichever is
// first. Later answers are ignored. The callbacks run on different threads.
class ReadinessCheck {
public:
    explicit ReadinessCheck(Respond respond) : respond_(std::move(respond)) {}

    void postgresAnswered(bool ok) { record(postgres_, ok); }
    void redisAnswered(bool ok) { record(redis_, ok); }

    void timedOut() {
        const std::lock_guard lock(mutex_);
        finish();
    }

private:
    enum class State { Waiting, Ok, Failed };

    void record(State& service, bool ok) {
        const std::lock_guard lock(mutex_);
        service = ok ? State::Ok : State::Failed;
        if (postgres_ != State::Waiting && redis_ != State::Waiting) finish();
    }

    void finish() {
        if (!respond_) return;

        const bool ready = postgres_ == State::Ok && redis_ == State::Ok;
        Json::Value body;
        body["status"] = ready ? "ready" : "unavailable";
        body["postgres"] = postgres_ == State::Ok ? "ok" : "unavailable";
        body["redis"] = redis_ == State::Ok ? "ok" : "unavailable";

        const auto response = HttpResponse::newHttpJsonResponse(body);
        response->setStatusCode(ready ? drogon::k200OK : drogon::k503ServiceUnavailable);
        respond_(response);
        respond_ = nullptr;
    }

    std::mutex mutex_;
    Respond respond_;
    State postgres_ = State::Waiting;
    State redis_ = State::Waiting;
};

}  // namespace

void registerHealthRoutes(const drogon::orm::DbClientPtr& database,
                          const drogon::nosql::RedisClientPtr& redis) {
    drogon::app().registerHandler("/healthz",
                                  [](const HttpRequestPtr&, Respond&& respond) {
                                      const auto response = HttpResponse::newHttpResponse();
                                      response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                                      response->setBody("ok");
                                      respond(response);
                                  },
                                  {drogon::Get});

    drogon::app().registerHandler(
        "/readyz",
        [database, redis](const HttpRequestPtr& request, Respond&& respond) {
            const std::string id = requestIdOf(request);
            const auto check = std::make_shared<ReadinessCheck>(std::move(respond));

            database->execSqlAsync(
                "SELECT 1", [check](const drogon::orm::Result&) { check->postgresAnswered(true); },
                [check, id](const drogon::orm::DrogonDbException& e) {
                    vec::logError("readiness: Postgres failed", id, {{"error", e.base().what()}});
                    check->postgresAnswered(false);
                });
            redis->execCommandAsync(
                [check](const drogon::nosql::RedisResult&) { check->redisAnswered(true); },
                [check, id](const std::exception& e) {
                    vec::logError("readiness: Redis failed", id, {{"error", e.what()}});
                    check->redisAnswered(false);
                },
                "PING");
            drogon::app().getLoop()->runAfter(readinessTimeoutSeconds, [check] { check->timedOut(); });
        },
        {drogon::Get});
}
