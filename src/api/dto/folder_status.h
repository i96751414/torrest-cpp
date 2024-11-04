#ifndef TORREST_FOLDER_STATUS_H
#define TORREST_FOLDER_STATUS_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FolderStatus : public oatpp::DTO {
    DTO_INIT(FolderStatus, DTO)

    FIELD(Int64, total, "Total size")

    FIELD(Int64, total_done, "Total done size")

    FIELD(Float64, progress, "Progress")

    FIELD(Int32, wanted_count, "Wanted files count")
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FOLDER_STATUS_H
