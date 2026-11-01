#ifndef HEALTH_ROUTES_H
#define HEALTH_ROUTES_H

#include <drogon/nosql/RedisClient.h>
#include <drogon/orm/DbClient.h>

// GET /healthz: 200 while the process is alive.
// GET /readyz: 200 if Postgres and Redis both answer in time, 503 if not. The body says which.
void registerHealthRoutes(const drogon::orm::DbClientPtr& database,
                          const drogon::nosql::RedisClientPtr& redis);

#endif  // HEALTH_ROUTES_H
