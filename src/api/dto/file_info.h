#ifndef TORREST_FILE_INFO_H
#define TORREST_FILE_INFO_H

#include "api/dto/utils.h"
#include "bittorrent/file.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FileInfo : public oatpp::DTO {
    DTO_INIT(FileInfo, DTO)

    FIELD(Int32, id, "File id")

    FIELD(Int64, lenght, "File size")

    FIELD(String, path, "File path")

    FIELD(String, name, "File name")

    static oatpp::data::mapping::type::DTOWrapper<FileInfo> create(const bittorrent::FileInfo &pInfo) {
        auto info = FileInfo::createShared();
        info->id = pInfo.id;
        info->lenght = pInfo.length;
        info->path = pInfo.path.c_str();
        info->name = pInfo.name.c_str();
        return info;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FILE_INFO_H
