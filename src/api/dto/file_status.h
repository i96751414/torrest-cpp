#ifndef TORREST_FILE_STATUS_H
#define TORREST_FILE_STATUS_H

#include "api/dto/utils.h"
#include "bittorrent/file.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class FileStatus : public oatpp::DTO {
    DTO_INIT(FileStatus, DTO)

    FIELD(Int64, total, "Total size")

    FIELD(Int64, total_done, "Total done size")

    FIELD(Float64, progress, "File progress")

    FIELD(UInt8, priority, "File priority")

    FIELD(Int64, buffering_total, "Total buffer size")

    FIELD(Float64, buffering_progress, "Buffering progress")

    FIELD(Int32, state, "File state")

    static oatpp::data::mapping::type::DTOWrapper<FileStatus> create(const bittorrent::FileStatus &pInfo) {
        auto status = FileStatus::createShared();
        status->total=pInfo.total;
        status->total_done=pInfo.total_done;
        status->progress=pInfo.progress;
        status->priority=pInfo.priority;
        status->buffering_total=pInfo.buffering_total;
        status->buffering_progress=pInfo.buffering_progress;
        status->state=pInfo.state;
        return status;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_FILE_STATUS_H
