#ifndef TORREST_FILE_INFO_STATUS_H
#define TORREST_FILE_INFO_STATUS_H

#include "api/dto/file_info.h"
#include "api/dto/file_status.h"
#include "api/dto/utils.h"
#include "bittorrent/file.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FileInfoStatus : public FileInfo {
    DTO_INIT(FileInfoStatus, FileInfo)

    FIELD(Object<FileStatus>, status, "File status")

    static oatpp::data::mapping::type::DTOWrapper<FileInfoStatus> create(const bittorrent::FileInfo &pInfo) {
        auto info = FileInfoStatus::createShared();
        info->id = pInfo.id;
        info->length = pInfo.length;
        info->path = pInfo.path.c_str();
        info->name = pInfo.name.c_str();
        info->status = nullptr;
        return info;
    }

    static oatpp::data::mapping::type::DTOWrapper<FileInfoStatus> create(const bittorrent::FileInfo &pInfo,
                                                                         const bittorrent::FileStatus &pStatus) {
        auto info = create(pInfo);
        info->status = FileStatus::create(pStatus);
        return info;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FILE_INFO_STATUS_H
