#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include <atomic>
#include <mutex>
#include <memory>

#include "spdlog/spdlog.h"
#include "libtorrent/torrent_handle.hpp"

#include "fwd.h"
#include "enums.h"

namespace torrest {

    class Torrent : std::enable_shared_from_this<Torrent> {
        friend class Service;

        friend class File;

    public:
        Torrent(const std::weak_ptr<Service> &pService, libtorrent::torrent_handle pHandle, std::string pInfoHash);

        void pause();

        void resume();

        void check_available_space();

        void check_save_resume_data();

        State get_state();

        double get_files_progress();

        const std::string &get_info_hash() const {
            return mInfoHash;
        }

    private:
        void handle_metadata_received();

        std::weak_ptr<Service> mService;
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

}

#endif //TORREST_TORRENT_H
