#ifndef EXPORT_ROUTES_H
#define EXPORT_ROUTES_H

#include <drogon/orm/DbClient.h>

// All behind AuthFilter. An export job is {"id", "documentId", "format", "status", "attempts",
// "resultUrl", "error", "createdAt"}; status is queued, running, done or failed, resultUrl is set once
// done and error once failed, and both are null until then. A worker does the export.
//
// POST /api/docs/{id}/exports {"format": "svg" or "png"} (viewer) -> 202 job, with Location
// /api/exports/{jobId}. If an export of the document in that format is already queued, that job is
// returned instead of a new one. 400 invalid_format; 404 not_found as for any document route.
//
// GET /api/exports/{id} -> 200 job, for any member of its document; 404 not_found otherwise, or if
// there's no such job.
void registerExportRoutes(const drogon::orm::DbClientPtr& database);

#endif  // EXPORT_ROUTES_H
