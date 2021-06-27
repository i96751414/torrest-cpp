#ifndef TORREST_TORRENT_INFO_H
#define TORREST_TORRENT_INFO_H

#include "api/dto/utils.h"
#include "api/dto/torrent_status.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class TorrentInfo : public oatpp::DTO {
    DTO_INIT(TorrentInfo, DTO)

    FIELD(String, info_hash, "Torrent info hash")

    FIELD(String, name, "Torrent name")

    FIELD(Int64, size, "Torrent total size")

    FIELD(Object<TorrentStatus>, status, "Torrent status")

    static oatpp::data::mapping::type::DTOWrapper<TorrentInfo> create(const bittorrent::TorrentInfo &pInfo) {
        auto info = TorrentInfo::createShared();
        info->info_hash = pInfo.info_hash.c_str();
        info->name = pInfo.name.c_str();
        info->size = pInfo.size;
        info->status = nullptr;
        return info;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_TORRENT_INFO_H
