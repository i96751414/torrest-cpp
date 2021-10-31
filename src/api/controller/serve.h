#ifndef TORREST_SERVE_CONTROLLER_H
#define TORREST_SERVE_CONTROLLER_H

#include "boost/filesystem.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "range_parser/range_parser.hpp"

#include "utils/mime.h"
#include "api/body/empty_body.h"
#include "api/body/reader_body.h"

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
        auto logger = ApiLogger::get_instance()->get_logger();
        auto file = GET_FILE(pInfoHash, pFile);
        auto mime = utils::guess_mime_type(boost::filesystem::path(file->get_name()).extension().string());
        logger->trace("operation=serve, infoHash={}, name='{}', mime='{}'", pInfoHash->c_str(), file->get_name(), mime);

        auto code = Status::CODE_200;
        std::shared_ptr<oatpp::web::protocol::http::outgoing::Body> body = nullptr;
        std::vector<std::pair<String, String>> headers = {{Header::CONTENT_TYPE, mime.c_str()}};
        auto isHead = pRequest->getStartingLine().method == "HEAD";

        auto rangeHeader = pRequest->getHeader(Header::RANGE);
        if (rangeHeader != nullptr) {
            auto range = range_parser::parse(rangeHeader, file->get_size());
            if (range.unit != range_parser::UNIT_BYTES) {
                return createDtoResponse(Status::CODE_416, MessageResponse::create("Invalid range"));
            }

            auto rangeCount = range.ranges.size();
            if (rangeCount == 1) {
                auto singleRange = range.ranges.at(0);
                code = Status::CODE_206;

                if (isHead) {
                    body = std::make_shared<EmptyBody>(singleRange.length);
                } else {
                    auto reader = file->reader();
                    reader->seek(singleRange.start, std::ios::beg);
                    body = std::make_shared<ReaderBody>(reader, singleRange.length);
                }

                headers.emplace_back(Header::CONTENT_RANGE, singleRange.content_range(file->get_size()).c_str());
            } else if (rangeCount > 1) {
                logger->error("operation=serve, message='Multiple ranges are not supported'");
                return createDtoResponse(Status::CODE_500, MessageResponse::create("Multi ranges are not supported"));
            }

            headers.emplace_back("Accept-Ranges", range_parser::UNIT_BYTES);
        }

        if (body == nullptr) {
            if (isHead) {
                body = std::make_shared<EmptyBody>(file->get_size());
            } else {
                body = std::make_shared<ReaderBody>(file->reader(), file->get_size());
            }
        }

        auto response = OutgoingResponse::createShared(code, body);
        for (auto &header : headers) {
            response->putHeader(header.first, header.second);
        }

        return response;
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_SERVE_CONTROLLER_H
