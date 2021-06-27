#ifndef TORREST_NEW_TORRENT_RESPONSE_H
#define TORREST_NEW_TORRENT_RESPONSE_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/data/mapping/type/Object.hpp"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class NewTorrentResponse : public oatpp::DTO {
    DTO_INIT(NewTorrentResponse, DTO)

    DTO_FIELD_INFO(info_hash) {
        info->description = "The torrent info hash";
    }

    DTO_FIELD(String, info_hash);

    static oatpp::data::mapping::type::DTOWrapper<NewTorrentResponse> create(const std::string &pInfoHash) {
        auto response = NewTorrentResponse::createShared();
        response->info_hash = pInfoHash.c_str();
        return response;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_NEW_TORRENT_RESPONSE_H
