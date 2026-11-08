#ifndef EXPORT_ROUTES_H
#define EXPORT_ROUTES_H

#include <drogon/orm/DbClient.h>

// All behind AuthFilter. An export job is {"id", "documentId", "format", "status", "attempts",
// "resultUrl", "error", "createdAt"}; status is queued, running, done or failed. A worker does the export,
// retrying a failed attempt after 5, 10, 20 and 40 s; after 5 attempts the job is failed. resultUrl is set
// once done; error is why the latest attempt failed, null until one has.
//
// POST /api/docs/{id}/exports {"format": "svg" or "png"} (viewer) -> 202 job, with Location
// /api/exports/{jobId}. If an export of the document in that format is already queued, that job is
// returned instead of a new one. 400 invalid_format; 404 not_found as for any document route.
//
// GET /api/exports/{id} -> 200 job, for any member of its document; 404 not_found otherwise, or if
// there's no such job.
//
// GET /api/exports/{id}/file -> 200 the file, as an attachment named drawing.svg or drawing.png, for
// any member of its document; 404 as above. 409 not_ready unless the job is done; 410 gone if the file
// has been deleted. Files are read from `exportDir`, where the worker saves them.
void registerExportRoutes(const drogon::orm::DbClientPtr& database, const std::string& exportDir);

#endif  // EXPORT_ROUTES_H
