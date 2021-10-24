#ifndef TORREST_FILES_CONTROLLER_H
#define TORREST_FILES_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "api/dto/message_response.h"
#include "api/dto/error_response.h"
#include "api/dto/file_info.h"
#include "torrest.h"

#define GET_FILE(infoHash, file) Torrest::get_instance().get_service()->get_torrent(infoHash)->get_file(file)

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class FilesController : public oatpp::web::server::api::ApiController {
public:
    explicit FilesController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    ENDPOINT_INFO(downloadFile) {
        info->summary = "Download file";
        info->description = "Download file from torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->pathParams.add<String>("file").description = "File index";
        info->queryParams.add<Boolean>("buffer").description = "Start buffering";
        info->queryParams.add<Boolean>("buffer").required = false;
        info->addResponse<List<Object<MessageResponse>>>(Status::CODE_200, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_400, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files/{file}/download", downloadFile,
             PATH(String, infoHash, "infoHash"),
             PATH(Int32, file, "file"),
             QUERY(Boolean, buffer, "buffer", "false")) {
        auto f = GET_FILE(infoHash, file);
        f->set_priority(libtorrent::default_priority);
        if (buffer) {
            f->buffer(std::max(f->get_size() / 200, Torrest::get_instance().get_buffer_size()), 10 * 1024 * 1024);
        }
        return createDtoResponse(Status::CODE_200, MessageResponse::create("File downloading"));
    }

    ENDPOINT_INFO(stopFile) {
        info->summary = "Stop file download";
        info->description = "Stop file download from torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->pathParams.add<String>("file").description = "File index";
        info->addResponse<List<Object<MessageResponse>>>(Status::CODE_200, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_400, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files/{file}/stop", stopFile,
             PATH(String, infoHash, "infoHash"),
             PATH(Int32, file, "file")) {
        GET_FILE(infoHash, file)->set_priority(libtorrent::dont_download);
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Stopped file download"));
    }

    ENDPOINT_INFO(fileInfo) {
        info->summary = "Get file info";
        info->description = "Get file info from torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->pathParams.add<String>("file").description = "File index";
        info->addResponse<List<Object<FileInfo>>>(Status::CODE_200, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_400, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files/{file}/info", fileInfo,
             PATH(String, infoHash, "infoHash"),
             PATH(Int32, file, "file")) {
        return createDtoResponse(Status::CODE_200, FileInfo::create(GET_FILE(infoHash, file)->get_info()));
    }

    ENDPOINT_INFO(fileStatus) {
        info->summary = "Get file status";
        info->description = "Get file status from torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->pathParams.add<String>("file").description = "File index";
        info->addResponse<List<Object<FileStatus>>>(Status::CODE_200, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_400, "application/json");
        info->addResponse<List<Object<ErrorResponse>>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files/{file}/status", fileStatus,
             PATH(String, infoHash, "infoHash"),
             PATH(Int32, file, "file")) {
        return createDtoResponse(Status::CODE_200, FileStatus::create(GET_FILE(infoHash, file)->get_status()));
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_FILES_CONTROLLER_H
