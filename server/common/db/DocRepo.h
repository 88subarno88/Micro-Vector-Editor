#ifndef SERVER_DOC_REPO_H
#define SERVER_DOC_REPO_H

#include <drogon/orm/DbClient.h>
#include <drogon/utils/coroutine.h>

#include <optional>
#include <string>
#include <tl/expected.hpp>
#include <utility>
#include <vector>

#include "db/Models.h"

namespace vec {

// One page of a user's documents, newest first.
struct DocPage {
    std::vector<Document> documents;
    std::optional<std::string> nextCursor;  // pass to list() for the next page; unset on the last
};

enum class ListDocsError { BadCursor };

// Reads and writes the documents table. An `id` that isn't a UUID finds nothing. Unexpected database
// failures throw DrogonDbException.
class DocRepo {
public:
    static constexpr int maxPageSize = 100;

    explicit DocRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // Also makes the owner a member, in the same statement.
    drogon::Task<Document> create(const std::string& ownerId, const std::string& title);
    drogon::Task<std::optional<Document>> get(const std::string& id);

    // Documents `userId` is a member of. `limit` is clamped to 1..maxPageSize. A page stays correct
    // if documents are added or removed between calls.
    drogon::Task<tl::expected<DocPage, ListDocsError>> list(const std::string& userId, int limit,
                                                            const std::optional<std::string>& cursor = {});

    // Return the updated document, or nullopt / false if there is no such document.
    drogon::Task<std::optional<Document>> rename(const std::string& id, const std::string& title);
    drogon::Task<bool> remove(const std::string& id);

private:
    drogon::orm::DbClientPtr database_;
};

// Reads and writes the document_members table.
class MemberRepo {
public:
    explicit MemberRepo(drogon::orm::DbClientPtr database) : database_(std::move(database)) {}

    // False, changing nothing, if the user is already a member or the user or document doesn't
    // exist. An existing member's role is never overwritten.
    drogon::Task<bool> add(const std::string& documentId, const std::string& userId, Role role);

    // nullopt if the user isn't a member.
    drogon::Task<std::optional<Role>> roleOf(const std::string& documentId, const std::string& userId);

private:
    drogon::orm::DbClientPtr database_;
};

}  // namespace vec

#endif  // SERVER_DOC_REPO_H
