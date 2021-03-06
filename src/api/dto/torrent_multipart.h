#ifndef TORREST_TORRENT_MULTIPART_H
#define TORREST_TORRENT_MULTIPART_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class TorrentMultipart : public oatpp::DTO {
    DTO_INIT(TorrentMultipart, DTO)

    FIELD(Binary, torrent, "The torrent file");
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_TORRENT_MULTIPART_H
