#ifndef TORREST_NEW_TORRENT_RESPONSE_H
#define TORREST_NEW_TORRENT_RESPONSE_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class NewTorrentResponse : public oatpp::DTO {
    DTO_INIT(NewTorrentResponse, DTO)

    FIELD(String, info_hash, "The torrent info hash")

    static oatpp::data::mapping::type::DTOWrapper<NewTorrentResponse> create(const std::string &pInfoHash) {
        auto response = NewTorrentResponse::createShared();
        response->info_hash = pInfoHash;
        return response;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_NEW_TORRENT_RESPONSE_H
