#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include <atomic>
#include <mutex>
#include <memory>

#include "spdlog/spdlog.h"
#include "libtorrent/torrent_handle.hpp"

#include "fwd.h"
#include "enums.h"

namespace torrest { namespace bittorrent {

    struct TorrentInfo {
        std::string info_hash;
        std::string name;
        std::int64_t size;
    };

    struct TorrentStatus {
        std::int64_t total;
        std::int64_t total_done;
        std::int64_t total_wanted;
        std::int64_t total_wanted_done;
        double progress;
        int download_rate;
        int upload_rate;
        bool paused;
        bool has_metadata;
        State state;
        int seeders;
        int seeders_total;
        int peers;
        int peers_total;
        std::int64_t seeding_time;
        std::int64_t finished_time;
        std::int64_t active_time;
        std::int64_t all_time_download;
        std::int64_t all_time_upload;
    };

    class Torrent : public std::enable_shared_from_this<Torrent> {
        friend class Service;

        friend class File;

    public:
        Torrent(libtorrent::torrent_handle pHandle, std::string pInfoHash, std::shared_ptr<spdlog::logger> pLogger);

        void pause();

        void resume();

        void check_available_space(const std::string &pPath);

        void check_save_resume_data();

        TorrentInfo get_info();

        TorrentStatus get_status();

        State get_state();

        const std::string &get_info_hash() const {
            return mInfoHash;
        }

    private:
        void handle_metadata_received();

        std::int64_t get_bytes_missing(const std::vector<libtorrent::piece_index_t> &pPieces);

        std::shared_ptr<spdlog::logger> mLogger;
        libtorrent::torrent_handle mHandle;
        std::string mInfoHash;
        std::string mDefaultName;
        std::vector<std::shared_ptr<File>> mFiles;
        std::mutex mMutex;
        std::mutex mFilesMutex;
        std::atomic<bool> mPaused{};
        std::atomic<bool> mHasMetadata;
        std::atomic<bool> mClosed;
    };

}}

#endif //TORREST_TORRENT_H
