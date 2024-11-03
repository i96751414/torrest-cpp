#ifndef TORREST_FOLDER_INFO_STATUS_H
#define TORREST_FOLDER_INFO_STATUS_H

#include "api/dto/folder_info.h"
#include "api/dto/folder_status.h"
#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FolderInfoStatus : public FolderInfo {
    DTO_INIT(FolderInfoStatus, FolderInfo)

    FIELD(Object<FolderStatus>, status, "Folder status")
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FOLDER_INFO_STATUS_H
