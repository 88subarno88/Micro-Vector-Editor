#include "ApiResponse.h"

drogon::HttpResponsePtr jsonResponse(drogon::HttpStatusCode status, const nlohmann::json& body) {
    const auto response = drogon::HttpResponse::newHttpResponse();
    response->setStatusCode(status);
    response->setContentTypeCode(drogon::CT_APPLICATION_JSON);
    response->setBody(body.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace));
    return response;
}

drogon::HttpResponsePtr errorResponse(drogon::HttpStatusCode status, const std::string& code,
                                      const std::string& message) {
    return jsonResponse(status, {{"error", {{"code", code}, {"message", message}}}});
}
