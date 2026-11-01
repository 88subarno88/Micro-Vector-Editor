#ifndef SERVER_MODELS_H
#define SERVER_MODELS_H

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>

namespace vec {

// One row of each table, as plain data. IDs are UUIDs and times are timestamps, both as Postgres
// writes them in text.

// Whether `text` is a UUID in Postgres's usual form: 8-4-4-4-12 hex digits.
bool isUuid(std::string_view text);

struct User {
    std::string id;
    std::string email;
    std::string passwordHash;  // never send this to a client or write it to a log
    std::string createdAt;
};

struct Document {
    std::string id;
    std::string ownerId;
    std::string title;
    int64_t headSeq = 0;  // sequence number of the latest applied operation
    std::string createdAt;
    std::string updatedAt;
};

// Ordered from least to most access, so that roles can be compared with <.
enum class Role { Viewer, Editor, Owner };

std::string_view toString(Role role);
std::optional<Role> roleFromString(std::string_view text);

struct Member {
    std::string documentId;
    std::string userId;
    Role role = Role::Viewer;
};

struct OpRow {
    std::string documentId;
    int64_t seq = 0;
    std::string userId;
    std::string clientOpId;
    nlohmann::json payload;  // a vec_core Operation
    std::string createdAt;
};

}  // namespace vec

#endif  // SERVER_MODELS_H
