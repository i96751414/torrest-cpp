#ifndef TORREST_TORRENTS_CONTROLLER_H
#define TORREST_TORRENTS_CONTROLLER_H

#include "boost/filesystem.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include "api/dto/file_info_status.h"
#include "api/dto/items.h"
#include "api/dto/torrent_info_status.h"
#include "torrest.h"

#define GET_TORRENT(infoHash) Torrest::get_instance()->get_service()->get_torrent(infoHash)

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

    ENDPOINT("GET", "/torrents", listTorrents, QUERY(Boolean, status, "status", false)) {
        auto torrents = Torrest::get_instance()->get_service()->get_torrents();
        auto responseList = List<Object<TorrentInfoStatus>>::createShared();
        for (const auto &torrent: torrents) {
            responseList->push_back(status ? TorrentInfoStatus::create(torrent->get_info(), torrent->get_status())
                                           : TorrentInfoStatus::create(torrent->get_info()));
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

    ENDPOINT("DELETE", "/torrents/{infoHash}", removeTorrent,
             PATH(String, infoHash, "infoHash"),
             QUERY(Boolean, deleteFiles, "delete", true)) {
        Torrest::get_instance()->get_service()->remove_torrent(infoHash, deleteFiles);
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Torrent removed"));
    }

    ENDPOINT_INFO(resumeTorrent) {
        info->summary = "Resume torrent";
        info->description = "Resume a paused torrent";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/resume", resumeTorrent, PATH(String, infoHash, "infoHash")) {
        GET_TORRENT(infoHash)->resume();
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Torrent resumed"));
    }

    ENDPOINT_INFO(pauseTorrent) {
        info->summary = "Pause torrent";
        info->description = "Pause torrent from service";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/pause", pauseTorrent, PATH(String, infoHash, "infoHash")) {
        GET_TORRENT(infoHash)->pause();
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
        return createDtoResponse(Status::CODE_200, TorrentInfo::create(GET_TORRENT(infoHash)->get_info()));
    }

    ENDPOINT_INFO(torrentStatus) {
        info->summary = "Torrent status";
        info->description = "Get torrent status";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<TorrentStatus>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/status", torrentStatus, PATH(String, infoHash, "infoHash")) {
        return createDtoResponse(Status::CODE_200, TorrentStatus::create(GET_TORRENT(infoHash)->get_status()));
    }

    ENDPOINT_INFO(torrentFiles) {
        info->summary = "Torrent files";
        info->description = "Get torrent files";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->queryParams.add<String>("prefix").description = "Filter result by prefix";
        info->queryParams.add<String>("prefix").required = false;
        info->queryParams.add<Boolean>("status").description = "Get files status";
        info->queryParams.add<Boolean>("status").required = false;
        info->addResponse<List<Object<FileInfoStatus>>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/files", torrentFiles,
             PATH(String, infoHash, "infoHash"),
             QUERY(String, prefixEscaped, "prefix", ""),
             QUERY(Boolean, status, "status", false)) {
        auto files = GET_TORRENT(infoHash)->get_files();
        auto responseList = oatpp::List<Object<FileInfoStatus>>::createShared();
        auto prefix = utils::unescape_string(prefixEscaped);

        for (const auto &file : files) {
            if (prefix.empty() || file->get_path().rfind(prefix, 0) == 0) {
                responseList->push_back(status ? FileInfoStatus::create(file->get_info(), file->get_status())
                                               : FileInfoStatus::create(file->get_info()));
            }
        }

        return (!prefix.empty() && responseList->empty())
                       ? createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid prefix provided"))
                       : createDtoResponse(Status::CODE_200, responseList);
    }

    ENDPOINT_INFO(torrentItems) {
        info->summary = "Torrent items";
        info->description = "Get torrent items";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->queryParams.add<String>("folder").description = "Items folder";
        info->queryParams.add<String>("folder").required = false;
        info->queryParams.add<Boolean>("status").description = "Get files status";
        info->queryParams.add<Boolean>("status").required = false;
        info->addResponse<Object<Items>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/items", torrentItems,
             PATH(String, infoHash, "infoHash"),
             QUERY(String, prefix, "folder", ""),
             QUERY(Boolean, status, "status", false)) {
        auto fileInfoList = List<Object<FileInfoStatus>>::createShared();
        auto folderInfoList = List<Object<FolderInfoStatus>>::createShared();

        // Normalize the prefix to remove any trailing slash
        boost::filesystem::path prefixPath(utils::unescape_string(prefix));
        if (prefixPath.has_filename() && prefixPath.filename() == ".") {
            prefixPath.remove_filename();
        }

        auto files = GET_TORRENT(infoHash)->get_files();
        for (const auto &file : files) {
            boost::filesystem::path filePath(file->get_path());

            // Check if filePath starts with prefix
            auto mismatch = std::mismatch(prefixPath.begin(), prefixPath.end(),
                                          filePath.begin(), filePath.end());

            // If no mismatch, it means prefix is a leading path in filePath
            if (mismatch.first == prefixPath.end()) {
                // Count the number of components in the relative path
                auto numComponents = std::distance(mismatch.second, filePath.end());

                if (numComponents == 1) {
                    // If there's only one component, it's a file at the prefix level
                    fileInfoList->push_back(status ? FileInfoStatus::create(file->get_info(), file->get_status())
                                                   : FileInfoStatus::create(file->get_info()));
                } else if (numComponents > 1) {
                    // If there are more than one component, add the first component as a directory
                    auto folderName = *mismatch.second;
                    auto folderPath = prefixPath / folderName;
                    // Keep the trailing slash to mark this as directory
                    folderPath += boost::filesystem::path::preferred_separator;

                    // Build or update our response
                    auto it = std::find_if(folderInfoList->begin(), folderInfoList->end(),
                                 [folderPath](const oatpp::data::mapping::type::DTOWrapper<FolderInfoStatus> &f)
                                           { return f->path == folderPath.string(); });

                    if (it == folderInfoList->end()) {
                        auto folderInfoStatus = FolderInfoStatus::createShared();
                        folderInfoStatus->name = folderName.string();
                        folderInfoStatus->path = folderPath.string();
                        folderInfoStatus->length = file->get_size();
                        folderInfoStatus->file_count = 1;

                        if (status) {
                            auto fileStatus = file->get_status();
                            folderInfoStatus->status = FolderStatus::createShared();
                            folderInfoStatus->status->total = fileStatus.total;
                            folderInfoStatus->status->total_done = fileStatus.total_done;
                            folderInfoStatus->status->progress = 100.0 * static_cast<double>(folderInfoStatus->status->total_done)
                                                                 / static_cast<double>(folderInfoStatus->status->total);
                        }

                        folderInfoList->push_back(folderInfoStatus);
                    } else {
                        (*it)->length = (*it)->length + file->get_size();
                        (*it)->file_count = (*it)->file_count + 1;

                        if (status) {
                            auto fileStatus = file->get_status();
                            (*it)->status->total = (*it)->status->total + fileStatus.total;
                            (*it)->status->total_done = (*it)->status->total_done + fileStatus.total_done;
                            (*it)->status->progress = 100.0 * static_cast<double>((*it)->status->total_done)
                                                      / static_cast<double>((*it)->status->total);
                        }
                    }
                }
            }
        }

        return (!prefix->empty() && folderInfoList->empty() && fileInfoList->empty())
                       ? createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid folder provided"))
                       : createDtoResponse(Status::CODE_200, Items::create(folderInfoList, fileInfoList));
    }

    ENDPOINT_INFO(torrentDownload) {
        info->summary = "Download";
        info->description = "Download all torrent files";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->queryParams.add<String>("prefix").description = "Download files by prefix";
        info->queryParams.add<String>("prefix").required = false;
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/download", torrentDownload,
             PATH(String, infoHash, "infoHash"),
             QUERY(String, prefixEscaped, "prefix", "")) {
        auto prefix = utils::unescape_string(prefixEscaped);

        if (prefix.empty()) {
            GET_TORRENT(infoHash)->set_priority(libtorrent::default_priority);
        } else {
            bool isDownloading = false;
            auto files = GET_TORRENT(infoHash)->get_files();

            for (const auto &file : files) {
                if (file->get_path().rfind(prefix, 0) == 0) {
                    file->set_priority(libtorrent::default_priority);
                    isDownloading = true;
                }
            }

            if (!isDownloading) {
                return createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid prefix provided"));
            }
        }

        return createDtoResponse(Status::CODE_200, MessageResponse::create("Download started"));
    }

    ENDPOINT_INFO(torrentStopDownload) {
        info->summary = "Stop download";
        info->description = "Stop downloading all torrent files";
        info->pathParams.add<String>("infoHash").description = "Torrent info hash";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/stop", torrentStopDownload,
             PATH(String, infoHash, "infoHash")) {
        GET_TORRENT(infoHash)->set_priority(libtorrent::dont_download);
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Stopped torrent download"));
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_TORRENTS_CONTROLLER_H
