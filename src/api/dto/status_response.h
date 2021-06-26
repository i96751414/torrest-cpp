#ifndef TORREST_STATUS_RESPONSE_H
#define TORREST_STATUS_RESPONSE_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/data/mapping/type/Object.hpp"

#include "bittorrent/service.h"

namespace torrest {

#include OATPP_CODEGEN_BEGIN(DTO)

class StatusResponse : public oatpp::DTO {
    DTO_INIT(StatusResponse, DTO)

    DTO_FIELD_INFO(progress) {
        info->description = "Overall progress";
    }

    DTO_FIELD(Float64, progress);

    DTO_FIELD_INFO(download_rate) {
        info->description = "Overall download rate";
    }

    DTO_FIELD(Int64, download_rate);

    DTO_FIELD_INFO(upload_rate) {
        info->description = "Overall upload rate";
    }

    DTO_FIELD(Int64, upload_rate);

    DTO_FIELD_INFO(num_torrents) {
        info->description = "Total number of torrents";
    }

    DTO_FIELD(Int32, num_torrents);

    DTO_FIELD_INFO(is_paused) {
        info->description = "Flag indicating if service is paused";
    }

    DTO_FIELD(Boolean, is_paused);

    static oatpp::data::mapping::type::DTOWrapper<StatusResponse> create(const ServiceStatus &pStatus) {
        auto status = StatusResponse::createShared();
        status->progress = pStatus.progress;
        status->download_rate = pStatus.download_rate;
        status->upload_rate = pStatus.upload_rate;
        status->num_torrents = pStatus.num_torrents;
        status->is_paused = pStatus.paused;
        return status;
    }
};

#include OATPP_CODEGEN_END(DTO)

}

#endif //TORREST_STATUS_RESPONSE_H
