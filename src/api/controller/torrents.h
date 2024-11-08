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
        info->queryParams["status"].description = "Get torrents status";
        info->queryParams["status"].required = false;
        info->addResponse<List<Object<TorrentInfoStatus>>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/torrents", listTorrents, QUERY(Boolean, status, "status", false)) {
        auto torrents = Torrest::get_instance()->get_service()->get_torrents();
        auto responseList = List<Object<TorrentInfoStatus>>::createShared();
        auto createTorrentInfoStatus = status
                ? [](const std::shared_ptr<bittorrent::Torrent> &t) { return TorrentInfoStatus::create(t->get_info(), t->get_status()); }
                : [](const std::shared_ptr<bittorrent::Torrent> &t) { return TorrentInfoStatus::create(t->get_info()); };

        for (const auto &torrent: torrents) {
            responseList->push_back(createTorrentInfoStatus(torrent));
        }

        return createDtoResponse(Status::CODE_200, responseList);
    }

    ENDPOINT_INFO(removeTorrent) {
        info->summary = "Remove torrent";
        info->description = "Remove torrent from service";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->queryParams["delete"].description = "Delete torrent files";
        info->queryParams["delete"].required = false;
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
        info->pathParams["infoHash"].description = "Torrent info hash";
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
        info->pathParams["infoHash"].description = "Torrent info hash";
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
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->addResponse<Object<TorrentInfo>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/info", torrentInfo, PATH(String, infoHash, "infoHash")) {
        return createDtoResponse(Status::CODE_200, TorrentInfo::create(GET_TORRENT(infoHash)->get_info()));
    }

    ENDPOINT_INFO(torrentStatus) {
        info->summary = "Torrent status";
        info->description = "Get torrent status";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->addResponse<Object<TorrentStatus>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
    }

    ENDPOINT("GET", "/torrents/{infoHash}/status", torrentStatus, PATH(String, infoHash, "infoHash")) {
        return createDtoResponse(Status::CODE_200, TorrentStatus::create(GET_TORRENT(infoHash)->get_status()));
    }

    ENDPOINT_INFO(torrentFiles) {
        info->summary = "Torrent files";
        info->description = "Get torrent files";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->queryParams["prefix"].description = "Filter result by prefix";
        info->queryParams["prefix"].required = false;
        info->queryParams["status"].description = "Get files status";
        info->queryParams["status"].required = false;
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
        auto createFileInfoStatus = status
                ? [](const std::shared_ptr<bittorrent::File> &f) { return FileInfoStatus::create(f->get_info(), f->get_status()); }
                : [](const std::shared_ptr<bittorrent::File> &f) { return FileInfoStatus::create(f->get_info()); };

        for (const auto &file : files) {
            if (prefix.empty() || file->get_path().rfind(prefix, 0) == 0) {
                responseList->push_back(createFileInfoStatus(file));
            }
        }

        return (!prefix.empty() && responseList->empty())
                       ? createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid prefix provided"))
                       : createDtoResponse(Status::CODE_200, responseList);
    }

    ENDPOINT_INFO(torrentItems) {
        info->summary = "Torrent items";
        info->description = "Get torrent items";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->queryParams["folder"].description = "Items folder";
        info->queryParams["folder"].required = false;
        info->queryParams["status"].description = "Get files status";
        info->queryParams["status"].required = false;
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
        auto createFileInfoStatus = status
                ? [](const std::shared_ptr<bittorrent::File> &f) { return FileInfoStatus::create(f->get_info(), f->get_status()); }
                : [](const std::shared_ptr<bittorrent::File> &f) { return FileInfoStatus::create(f->get_info()); };

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
                    fileInfoList->push_back(createFileInfoStatus(file));
                } else if (numComponents > 1) {
                    // If there are more than one component, add the first component as a directory
                    auto folderName = *mismatch.second;
                    auto folderPath = prefixPath / folderName;
                    // Keep the trailing slash to mark this as directory
                    folderPath += boost::filesystem::path::preferred_separator;

                    // Build or update our response
                    auto it = std::find_if(folderInfoList->rbegin(), folderInfoList->rend(),
                                 [folderPath](const oatpp::data::mapping::type::DTOWrapper<FolderInfoStatus> &f)
                                           { return f->path == folderPath.string(); });

                    if (it == folderInfoList->rend()) {
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
                            folderInfoStatus->status->wanted_count = fileStatus.priority ? 1 : 0;
                        }

                        folderInfoList->push_back(folderInfoStatus);
                    } else {
                        (*it)->length = (*it)->length + file->get_size();
                        (*it)->file_count = (*it)->file_count + 1;

                        if (status) {
                            auto fileStatus = file->get_status();
                            (*it)->status->total = (*it)->status->total + fileStatus.total;
                            (*it)->status->total_done = (*it)->status->total_done + fileStatus.total_done;
                            if (fileStatus.priority) (*it)->status->wanted_count = (*it)->status->wanted_count + 1;
                        }
                    }
                }
            }
        }

        if (status) {
            for (auto &folder : *folderInfoList) {
                auto &fs = folder->status;
                fs->progress = fs->total > 0 ? 100.0 * static_cast<double>(fs->total_done) / static_cast<double>(fs->total) : 100;
            }
        }

        return (!prefix->empty() && folderInfoList->empty() && fileInfoList->empty())
                       ? createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid folder provided"))
                       : createDtoResponse(Status::CODE_200, Items::create(folderInfoList, fileInfoList));
    }

    ENDPOINT_INFO(torrentDownload) {
        info->summary = "Start download";
        info->description = "Download torrent files";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->queryParams["prefix"].description = "Download files by prefix";
        info->queryParams["prefix"].required = false;
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/download", torrentDownload,
             PATH(String, infoHash, "infoHash"),
             QUERY(String, prefix, "prefix", "")) {
        return set_priority(libtorrent::default_priority, infoHash, utils::unescape_string(prefix), "Download started");
    }

    ENDPOINT_INFO(torrentStopDownload) {
        info->summary = "Stop download";
        info->description = "Stop downloading torrent files";
        info->pathParams["infoHash"].description = "Torrent info hash";
        info->queryParams["prefix"].description = "Stop files download by prefix";
        info->queryParams["prefix"].required = false;
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_404, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("PUT", "/torrents/{infoHash}/stop", torrentStopDownload,
             PATH(String, infoHash, "infoHash"),
             QUERY(String, prefix, "prefix", "")) {
        return set_priority(libtorrent::dont_download, infoHash, utils::unescape_string(prefix), "Download stopped");
    }

    std::shared_ptr<OutgoingResponse> set_priority(libtorrent::download_priority_t priority,
                                                   const std::string &infoHash,
                                                   const std::string &prefix,
                                                   const std::string &successMessage) {
        if (prefix.empty()) {
            GET_TORRENT(infoHash)->set_priority(priority);
        } else {
            bool isDownloading = false;
            auto files = GET_TORRENT(infoHash)->get_files();

            for (const auto &file : files) {
                if (file->get_path().rfind(prefix, 0) == 0) {
                    file->set_priority(priority);
                    isDownloading = true;
                }
            }

            if (!isDownloading) {
                return createDtoResponse(Status::CODE_404, ErrorResponse::create("Invalid prefix provided"));
            }
        }

        return createDtoResponse(Status::CODE_200, MessageResponse::create(successMessage));
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_TORRENTS_CONTROLLER_H
