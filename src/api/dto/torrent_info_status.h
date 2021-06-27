#ifndef TORREST_TORRENT_INFO_STATUS_H
#define TORREST_TORRENT_INFO_STATUS_H

#include "api/dto/utils.h"
#include "api/dto/torrent_info.h"
#include "api/dto/torrent_status.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class TorrentInfoStatus : public TorrentInfo {
    DTO_INIT(TorrentInfoStatus, TorrentInfo)

    FIELD(Object<TorrentStatus>, status, "Torrent status")

    static oatpp::data::mapping::type::DTOWrapper<TorrentInfoStatus> create(const bittorrent::TorrentInfo &pInfo) {
        auto info = TorrentInfoStatus::createShared();
        info->info_hash = pInfo.info_hash.c_str();
        info->name = pInfo.name.c_str();
        info->size = pInfo.size;
        info->status = nullptr;
        return info;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_TORRENT_INFO_STATUS_H
