#ifndef TORREST_SERVICE_CONTROLLER_H
#define TORREST_SERVICE_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "torrest.h"
#include "api/dto/message_response.h"
#include "api/dto/status_response.h"

namespace torrest {

#include OATPP_CODEGEN_BEGIN(ApiController)

class ServiceController : public oatpp::web::server::api::ApiController {
public:
    explicit ServiceController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    ENDPOINT_INFO(status) {
        info->summary = "Status";
        info->description = "Get the service status";
        info->addResponse<Object<StatusResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/status", status) {
        auto response = createDtoResponse(Status::CODE_200, StatusResponse::create(
                Torrest::get_instance().get_service()->get_status()));
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }

    ENDPOINT_INFO(pause) {
        info->summary = "Pause";
        info->description = "Pause the service";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/pause", pause) {
        Torrest::get_instance().get_service()->pause();
        auto response = createDtoResponse(Status::CODE_200, MessageResponse::create("Service paused"));
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }

    ENDPOINT_INFO(resume) {
        info->summary = "Resume";
        info->description = "Resume the service";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/resume", resume) {
        Torrest::get_instance().get_service()->resume();
        auto response = createDtoResponse(Status::CODE_200, MessageResponse::create("Service resumed"));
        response->putHeader(Header::CONTENT_TYPE, "application/json");
        return response;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}

#endif //TORREST_SERVICE_CONTROLLER_H
