#ifndef TORREST_SETTINGS_CONTROLLER_H
#define TORREST_SETTINGS_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "torrest.h"

namespace torrest {

#include OATPP_CODEGEN_BEGIN(ApiController)

class SettingsController : public oatpp::web::server::api::ApiController {
public:
    explicit SettingsController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    ENDPOINT_INFO(getSettings) {
        info->summary = "Get current settings";
        info->description = "Get settings as a JSON object";
        info->addResponse<oatpp::Any>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/settings/get", getSettings) {
        auto response = createResponse(Status::CODE_200, Torrest::get_instance().dump_settings().c_str());
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }

    ENDPOINT_INFO(setSettings) {
        info->summary = "Set settings";
        info->description = "Set settings given the provided JSON object";
        info->addConsumes<oatpp::Any>("application/json");
        info->addResponse<oatpp::Any>(Status::CODE_200, "application/json");
    }

    ENDPOINT("POST", "/settings/set", setSettings, BODY_STRING(String, body)) {
        // TODO: handle reset torrents & finish implementation
        Settings settings = Settings::parse(body->std_str());
        settings.validate();

        auto response = createResponse(Status::CODE_200, body);
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}

#endif //TORREST_SETTINGS_CONTROLLER_H
