#ifndef TORREST_SETTINGS_CONTROLLER_H
#define TORREST_SETTINGS_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "torrest.h"
#include "api/dto/error_response.h"
#include "api/logger.h"

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
        info->queryParams.add<Boolean>("reset").description = "Reset torrents";
        info->queryParams.add<Boolean>("reset").required = false;
        info->addConsumes<oatpp::Any>("application/json");
        info->addResponse<oatpp::Any>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_400, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("POST", "/settings/set", setSettings, QUERY(Boolean, reset, "reset", "false"), BODY_STRING(String, body)) {
        Settings settings = Settings::parse(body->std_str());
        settings.validate();

        Torrest::get_instance().update_settings(settings, reset);
        ApiLogger::get_instance()->get_logger()->set_level(settings.api_log_level);

        auto response = createResponse(Status::CODE_200, body);
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}

#endif //TORREST_SETTINGS_CONTROLLER_H
