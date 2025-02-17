#ifndef TORREST_SETTINGS_CONTROLLER_H
#define TORREST_SETTINGS_CONTROLLER_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include "api/dto/error_response.h"
#include "torrest.h"

namespace torrest { namespace api {

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

    ENDPOINT("GET", "/settings", getSettings) {
        auto response = createResponse(Status::CODE_200, Torrest::get_instance()->dump_settings());
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }

    ENDPOINT_INFO(setSettings) {
        info->summary = "Set settings";
        info->description = "Set settings given the provided JSON object";
        info->queryParams["reset"].description = "Reset torrents";
        info->queryParams["reset"].required = false;
        info->addConsumes<oatpp::Any>("application/json");
        info->addResponse<oatpp::Any>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_400, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("PUT", "/settings", setSettings, QUERY(Boolean, reset, "reset", false), BODY_STRING(String, body)) {
        Torrest::get_instance()->update_settings(body, reset);

        auto response = createResponse(Status::CODE_200, body);
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_SETTINGS_CONTROLLER_H
