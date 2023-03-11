#ifndef TORREST_TORRENT_STATUS_H
#define TORREST_TORRENT_STATUS_H

#include "api/dto/utils.h"
#include "bittorrent/torrent.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class TorrentStatus : public oatpp::DTO {
    DTO_INIT(TorrentStatus, DTO)

    FIELD(Int64, total, "Total size")

    FIELD(Int64, total_done, "Total done size")

    FIELD(Int64, total_wanted, "Total wanted size")

    FIELD(Int64, total_wanted_done, "Total wanted done size")

    FIELD(Float64, progress, "Total progress")

    FIELD(Int32, download_rate, "Total download rate")

    FIELD(Int32, upload_rate, "Total upload rate")

    FIELD(Boolean, paused, "Torrent is paused")

    FIELD(Boolean, has_metadata, "Torrent has metadata")

    FIELD(Int32, state, "State of the torrent")

    FIELD(Int32, seeders, "Number of seeders")

    FIELD(Int32, seeders_total, "Number of total seeders")

    FIELD(Int32, peers, "Number of peers")

    FIELD(Int32, peers_total, "Number of total peers")

    FIELD(Int64, seeding_time, "Seeding time (seconds)")

    FIELD(Int64, finished_time, "Finished time (seconds)")

    FIELD(Int64, active_time, "Active time (seconds)")

    FIELD(Int64, all_time_download, "All time download (seconds)")

    FIELD(Int64, all_time_upload, "All time upload (seconds)")

    static oatpp::data::mapping::type::DTOWrapper<TorrentStatus> create(const bittorrent::TorrentStatus &pStatus) {
        auto status = TorrentStatus::createShared();
        status->total = pStatus.total;
        status->total_done = pStatus.total_done;
        status->total_wanted = pStatus.total_wanted;
        status->total_wanted_done = pStatus.total_wanted_done;
        status->progress = pStatus.progress;
        status->download_rate = pStatus.download_rate;
        status->upload_rate = pStatus.upload_rate;
        status->paused = pStatus.paused;
        status->has_metadata = pStatus.has_metadata;
        status->state = pStatus.state;
        status->seeders = pStatus.seeders;
        status->seeders_total = pStatus.seeders_total;
        status->peers = pStatus.peers;
        status->peers_total = pStatus.peers_total;
        status->seeding_time = pStatus.seeding_time;
        status->finished_time = pStatus.finished_time;
        status->active_time = pStatus.active_time;
        status->all_time_download = pStatus.all_time_download;
        status->all_time_upload = pStatus.all_time_upload;
        return status;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_TORRENT_STATUS_H
