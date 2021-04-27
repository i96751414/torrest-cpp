#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include <atomic>
#include <mutex>
#include <memory>

#include "spdlog/spdlog.h"
#include "libtorrent/torrent_handle.hpp"

#include "fwd.h"

namespace torrest {

    enum Status {
        queued,
        checking,
        finding,
        downloading,
        finished,
        seeding,
        allocating,
        checking_resume_data,
        paused,
        buffering
    };

    class Torrent : std::enable_shared_from_this<Torrent> {
        friend class Service;

        friend class File;

    public:
        Torrent(const std::weak_ptr<Service> &pService, libtorrent::torrent_handle pHandle, std::string pInfoHash);

        std::string get_info_hash();

        void pause();

        void resume();

        void check_available_space();

        void check_save_resume_data();

    private:
        void handle_metadata_received();

        std::weak_ptr<Service> mService;
        std::shared_ptr<spdlog::logger> mLogger;
        libtorrent::torrent_handle mHandle;
        std::string mInfoHash;
        std::string mDefaultName;
        std::vector<std::shared_ptr<File>> mFiles;
        std::mutex mMutex;
        std::atomic<bool> mPaused{};
        std::atomic<bool> mHasMetadata;
        std::atomic<bool> mClosed;
    };

}

#endif //TORREST_TORRENT_H