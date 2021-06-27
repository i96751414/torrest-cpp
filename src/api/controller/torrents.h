#ifndef TORREST_TORRENTS_CONTROLLER_H
#define TORREST_TORRENTS_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"

#include "torrest.h"
#include "api/dto/torrent_info.h"
#include "api/dto/torrent_status.h"

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
        info->addResponse<List<Object<TorrentInfo>>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/torrents", listTorrents, QUERY(Boolean, status, "status", "false")) {
        auto torrents = Torrest::get_instance().get_service()->get_torrents();
        auto responseList = oatpp::List<Object<TorrentInfo>>::createShared();
        for (auto &torrent: torrents) {
            auto info = TorrentInfo::create(torrent->get_info());
            if (status) {
                info->status = TorrentStatus::create(torrent->get_status());
            }
            responseList->push_back(info);
        }
        return createDtoResponse(Status::CODE_200, responseList);
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_TORRENTS_CONTROLLER_H
