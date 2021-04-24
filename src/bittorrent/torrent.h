#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include <atomic>
#include <mutex>

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

    class Torrent {
        friend class Service;
        friend class File;

    public:
        Torrent(Service *pService, libtorrent::torrent_handle pHandle, std::string pInfoHash);

        ~Torrent();

        std::string get_info_hash();

    private:
        void handle_metadata_received();

        Service *mService;
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
