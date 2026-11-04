#ifndef DOC_ROUTES_H
#define DOC_ROUTES_H

#include <drogon/orm/DbClient.h>

// All behind AuthFilter. A document is {"id", "ownerId", "title", "headSeq", "createdAt", "updatedAt"}.
//
// POST /api/docs {"title"} -> 201 document, owned by the caller. 400 invalid_title unless the title is
// 1 to 200 bytes after trimming, with no control characters.
//
// POST /api/docs?title=... with Content-Type image/svg+xml and an SVG file as the body -> 201 document,
// whose drawing is the file's shapes, saved as its first snapshot. The title defaults to "Imported
// drawing". 400 invalid_svg for a file the parser rejects or whose text isn't UTF-8; bodies over 1 MB get
// 413 from Drogon.
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
//
// POST /api/docs/{id}/members {"email", "role": "editor" or "viewer"} (owner) -> 201 {"documentId",
// "userId", "email", "role"}. 400 invalid_role, 404 user_not_found, or 409 already_member; an existing
// member's role is never changed here.
void registerDocRoutes(const drogon::orm::DbClientPtr& database);

#endif  // DOC_ROUTES_H
