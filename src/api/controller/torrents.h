#ifndef TORREST_TORRENTS_CONTROLLER_H
#define TORREST_TORRENTS_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "torrest.h"
#include "api/dto/torrent_info_status.h"
#include "api/dto/file_info_status.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class TorrentsController : public oatpp::web::server::api::ApiController {
public:
    explicit TorrentsController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    ENDPOINT_INFO(listTorrents) {
        info->summary = "List torrents";
        info->description = "List all torrents from service";
        info->queryParams.add<Boolean>("status").description = "Get torrents status";
        info->queryParams.add<Boolean>("status").required = false;
        info->addResponse<List<Object<TorrentInfoStatus>>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/torrents", listTorrents, QUERY(Boolean, status, "status", "false")) {
        auto torrents = Torrest::get_instance().get_service()->get_torrents();
        auto responseList = oatpp::List<Object<TorrentInfoStatus>>::createShared();
        for (auto &torrent: torrents) {
            auto info = TorrentInfoStatus::create(torrent->get_info());
            if (status) {
                info->status = TorrentStatus::create(torrent->get_status());
            }
            responseList->push_back(info);
        }
        return createDtoResponse(Status::CODE_200, responseList);
    }

    ENDPOINT_INFO(removeTorrent) {
        info->summary = "Remove torrent";
        info->description = "Remove torrent from service";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->queryParams.add<Boolean>("delete").description = "Delete torrent files";
        info->queryParams.add<Boolean>("delete").required = false;
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/remove", removeTorrent,
             PATH(String, infoHash, "infoHash"),
             QUERY(Boolean, deleteFiles, "delete", "true")) {
        Torrest::get_instance().get_service()->remove_torrent(infoHash->std_str(), deleteFiles);
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Torrent removed"));
    }

    ENDPOINT_INFO(resumeTorrent) {
        info->summary = "Resume torrent";
        info->description = "Resume a paused torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/resume", resumeTorrent, PATH(String, infoHash, "infoHash")) {
        Torrest::get_instance().get_service()->get_torrent(infoHash->std_str())->resume();
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Torrent resumed"));
    }

    ENDPOINT_INFO(pauseTorrent) {
        info->summary = "Pause torrent";
        info->description = "Pause torrent from service";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/pause", pauseTorrent, PATH(String, infoHash, "infoHash")) {
        Torrest::get_instance().get_service()->get_torrent(infoHash->std_str())->pause();
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Torrent paused"));
    }

    ENDPOINT_INFO(torrentInfo) {
        info->summary = "Torrent info";
        info->description = "Get torrent info";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<TorrentInfo>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/info", torrentInfo, PATH(String, infoHash, "infoHash")) {
        return createDtoResponse(Status::CODE_200, TorrentInfo::create(
                Torrest::get_instance().get_service()->get_torrent(infoHash->std_str())->get_info()));
    }

    ENDPOINT_INFO(torrentStatus) {
        info->summary = "Torrent status";
        info->description = "Get torrent status";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<TorrentStatus>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/status", torrentStatus, PATH(String, infoHash, "infoHash")) {
        return createDtoResponse(Status::CODE_200, TorrentStatus::create(
                Torrest::get_instance().get_service()->get_torrent(infoHash->std_str())->get_status()));
    }

    ENDPOINT_INFO(torrentFiles) {
        info->summary = "Torrent files";
        info->description = "Get torrent files";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->queryParams.add<Boolean>("status").description = "Get files status";
        info->queryParams.add<Boolean>("status").required = false;
        info->addResponse<List<Object<FileInfoStatus>>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files", torrentFiles,
             PATH(String, infoHash, "infoHash"),
             QUERY(Boolean, status, "status", "false")) {
        auto files = Torrest::get_instance().get_service()->get_torrent(infoHash->std_str())->get_files();
        auto responseList = oatpp::List<Object<FileInfoStatus>>::createShared();
        for (auto &file : files) {
            auto info = FileInfoStatus::create(file->get_info());
            if (status) {
                info->status = FileStatus::create(file->get_status());
            }
            responseList->push_back(info);
        }
        return createDtoResponse(Status::CODE_200, responseList);
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_TORRENTS_CONTROLLER_H
