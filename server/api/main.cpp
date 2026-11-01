#include <drogon/drogon.h>

#include <iostream>

#include "config/Config.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponse;
using drogon::HttpResponsePtr;

int main() {
    const auto config = vec::loadConfig();
    if (!config) {
        std::cerr << config.error() << '\n';
        return 1;
    }

    drogon::app()
        .registerHandler("/healthz",
                         [](const HttpRequestPtr&, std::function<void(const HttpResponsePtr&)>&& respond) {
                             const auto response = HttpResponse::newHttpResponse();
                             response->setContentTypeCode(drogon::CT_TEXT_PLAIN);
                             response->setBody("ok");
                             respond(response);
                         },
                         {drogon::Get})
        .addListener("0.0.0.0", config->port)
        .run();
}
