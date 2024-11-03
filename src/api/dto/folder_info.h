#ifndef TORREST_FOLDER_INFO_H
#define TORREST_FOLDER_INFO_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FolderInfo : public oatpp::DTO {
    DTO_INIT(FolderInfo, DTO)

    FIELD(String, name, "Folder name")

    FIELD(String, path, "Folder path")

    FIELD(Int64, length, "Folder size")

    FIELD(Int32, file_count, "File count")
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FOLDER_INFO_H
