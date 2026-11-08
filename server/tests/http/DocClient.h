#ifndef SERVER_DOC_CLIENT_H
#define SERVER_DOC_CLIENT_H

#include <drogon/WebSocketClient.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <string>

#include "core/Diagram.h"
#include "ops/Operation.h"

// A client of /ws/docs/{id}, as a browser would be: it joins, sends ops, and keeps a replica of the
// drawing, applying in seq order the ops others made and its own once acknowledged. Rejected ops are
// never applied, so the replica only ever holds what the server committed.
class DocClient {
public:
    // Connects and joins; check joined() before use.
    DocClient(uint16_t port, const std::string& documentId, const std::string& token);
    ~DocClient();
    DocClient(const DocClient&) = delete;
    DocClient& operator=(const DocClient&) = delete;

    bool joined() const { return joined_; }

    // Sends `op` without waiting for the answer.
    void send(const vec::Operation& op);

    // Waits until every op sent has been acknowledged or rejected and the replica has applied every op up
    // to `seq` (0: no ops needed). False if that takes longer than `timeout`.
    bool waitUntilSettled(int64_t seq, std::chrono::seconds timeout = std::chrono::seconds(10));

    nlohmann::json replica();  // as diagramToJson
    int64_t lastSeq();
    int acked();
    int rejected();
    std::string problem();  // anything that shouldn't happen, such as a confirmed op that didn't apply

private:
    void handle(const std::string& message);
    void applyReady();  // applies the ops that are next in seq order

    drogon::WebSocketClientPtr socket_;
    bool joined_ = false;
    std::mutex mutex_;
    std::condition_variable changed_;
    Diagram diagram_;
    int64_t lastSeq_ = 0;
    std::map<int64_t, vec::Operation> confirmed_;    // ops by seq, waiting to be applied in order
    std::map<std::string, vec::Operation> pending_;  // sent and not yet answered, by clientOpId
    int acked_ = 0;
    int rejected_ = 0;
    std::string problem_;
};

#endif  // SERVER_DOC_CLIENT_H
