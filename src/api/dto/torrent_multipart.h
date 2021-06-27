#ifndef TORREST_TORRENT_MULTIPART_H
#define TORREST_TORRENT_MULTIPART_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp-swagger/Types.hpp"

namespace torrest {

#include OATPP_CODEGEN_BEGIN(DTO)

class TorrentMultipart : public oatpp::DTO {
    DTO_INIT(TorrentMultipart, DTO
    )

    DTO_FIELD_INFO(torrent) {
        info->description = "The torrent file";
    }

    DTO_FIELD(oatpp::swagger::Binary, torrent);
};

#include OATPP_CODEGEN_END(DTO)

}

#endif //TORREST_TORRENT_MULTIPART_H
