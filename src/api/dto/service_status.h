#ifndef TORREST_SERVICE_STATUS_H
#define TORREST_SERVICE_STATUS_H

#include "api/dto/utils.h"
#include "bittorrent/service.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class ServiceStatus : public oatpp::DTO {
    DTO_INIT(ServiceStatus, DTO)

    FIELD(Float64, progress, "Overall progress")

    FIELD(Int64, download_rate, "Overall download rate")

    FIELD(Int64, upload_rate, "Overall upload rate")

    FIELD(Int32, num_torrents, "Total number of torrents")

    FIELD(Boolean, is_paused, "Flag indicating if service is paused")

    static oatpp::data::mapping::type::DTOWrapper<ServiceStatus> create(const bittorrent::ServiceStatus &pStatus) {
        auto status = ServiceStatus::createShared();
        status->progress = pStatus.progress;
        status->download_rate = pStatus.download_rate;
        status->upload_rate = pStatus.upload_rate;
        status->num_torrents = pStatus.num_torrents;
        status->is_paused = pStatus.paused;
        return status;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_SERVICE_STATUS_H
