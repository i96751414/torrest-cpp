#ifndef TORREST_SERVE_CONTROLLER_H
#define TORREST_SERVE_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#define SERVE(method, name, code, serve)                                                        \
    ENDPOINT_INFO(name) {                                                                       \
        info->summary = "Serve file";                                                           \
        info->description = "Serve file from torrent";                                          \
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";             \
        info->pathParams.add<String>("file").description = "File index";                        \
        info->addResponse<List<Object<FileStatus>>>(code, "application/octet-stream");          \
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_400, "application/json");   \
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_404, "application/json");   \
    }                                                                                           \
                                                                                                \
    ENDPOINT(method, "/torrents/{infoHash}/files/{file}/serve", name,                           \
             PATH(String, infoHash, "infoHash"),                                                \
             PATH(Int32, file, "file"),                                                         \
             REQUEST(std::shared_ptr<IncomingRequest>, request)) {                              \
        return serve(request, infoHash, file);                                                  \
    }

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class ServeController : public oatpp::web::server::api::ApiController {
public:
    explicit ServeController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    SERVE("HEAD", serveFileHead, Status::CODE_200, serve)

    SERVE("GET", serveFileGet, Status::CODE_206, serve)

    std::shared_ptr<OutgoingResponse> serve(const std::shared_ptr<IncomingRequest> &pRequest,
                                            const String &pInfoHash,
                                            const Int32 &pFile) {
        auto file = GET_FILE(pInfoHash, pFile);
        return nullptr;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_SERVE_CONTROLLER_H
