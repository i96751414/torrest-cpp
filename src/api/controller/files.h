#ifndef TORREST_FILES_CONTROLLER_H
#define TORREST_FILES_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class FilesController : public oatpp::web::server::api::ApiController {
public:
    explicit FilesController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_FILES_CONTROLLER_H
