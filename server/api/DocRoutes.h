#ifndef DOC_ROUTES_H
#define DOC_ROUTES_H

#include <drogon/orm/DbClient.h>

// All behind AuthFilter. A document is {"id", "ownerId", "title", "headSeq", "createdAt", "updatedAt"}.
//
// POST /api/docs {"title"} -> 201 document, owned by the caller. 400 invalid_title unless the title is
// 1 to 200 bytes after trimming, with no control characters.
//
// GET /api/docs?limit=20&cursor= -> 200 {"documents": [...], "nextCursor": "..." or null}: documents the
// caller is a member of, newest first. limit is clamped to 1..100. 400 for a non-numeric limit or a
// cursor that didn't come from this endpoint.
//
// For one document, a non-member, or an ID that isn't a document, gets 404 not_found; a member whose
// role is too low gets 403 forbidden.
//
// GET /api/docs/{id} (viewer) -> 200 {"document", "role", "snapshot": {"seq", "state"} or null,
// "ops": [{"seq", "userId", "clientOpId", "op", "createdAt"}], "opsComplete"}: the newest snapshot and the
// ops after it, oldest first. opsComplete is false if there were too many to send at once.
//
// PATCH /api/docs/{id} {"title"} (editor) -> 200 document, renamed.
//
// DELETE /api/docs/{id} (owner) -> 204, deleting it with its members, ops and snapshots.
void registerDocRoutes(const drogon::orm::DbClientPtr& database);

#endif  // DOC_ROUTES_H
