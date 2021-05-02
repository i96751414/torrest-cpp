#include "torrent.h"

#include <experimental/filesystem>

#include "libtorrent/torrent_status.hpp"
#include "libtorrent/torrent_info.hpp"
#include "service.h"
#include "file.h"
#include "exceptions.h"

#define CHECK_SERVICE(s) if (!s) { throw torrest::InvalidServiceException("Invalid service"); }

namespace torrest {

    Torrent::Torrent(const std::weak_ptr<Service> &pService, libtorrent::torrent_handle pHandle, std::string pInfoHash)
            : mService(pService),
              mLogger(pService.lock()->mLogger),
              mHandle(std::move(pHandle)),
              mInfoHash(std::move(pInfoHash)),
              mHasMetadata(false),
              mClosed(false) {

        auto flags = mHandle.flags();
        auto status = mHandle.status(libtorrent::torrent_handle::query_name);

        mPaused = (flags & libtorrent::torrent_flags::paused) && !(flags & libtorrent::torrent_flags::auto_managed);
        mDefaultName = status.name.empty() ? mInfoHash : status.name;

        if (status.has_metadata) {
            handle_metadata_received();
        }
    }

    void Torrent::handle_metadata_received() {
        mLogger->debug("operation=handle_metadata_received");
        std::lock_guard<std::mutex> lock(mMutex);
        auto torrentFile = mHandle.torrent_file();
        auto files = torrentFile->files();

        mFiles.clear();
        for (int i = 0; i < torrentFile->num_files(); i++) {
            mFiles.emplace_back(std::make_shared<File>(weak_from_this(), files, libtorrent::file_index_t(i)));
        }

        mHasMetadata = true;
    }

    void Torrent::pause() {
        mLogger->debug("operation=pause, message='Pausing torrent', infoHash={}", mInfoHash);
        std::lock_guard<std::mutex> lock(mMutex);
        mHandle.unset_flags(libtorrent::torrent_flags::auto_managed);
        mHandle.pause(libtorrent::torrent_handle::clear_disk_cache);
        mPaused = true;
    }

    void Torrent::resume() {
        mLogger->debug("operation=resume, message='Resuming torrent', infoHash={}", mInfoHash);
        std::lock_guard<std::mutex> lock(mMutex);
        mHandle.set_flags(libtorrent::torrent_flags::auto_managed);
        mPaused = false;
    }

    void Torrent::check_available_space() {
        mLogger->debug("operation=check_available_space, message='Checking available space', infoHash={}", mInfoHash);
        auto service = mService.lock();
        CHECK_SERVICE(service)

        if (!service->mSettings.check_available_space) {
            return;
        }

        auto status = mHandle.status(libtorrent::torrent_handle::query_accurate_download_counters
                                     | libtorrent::torrent_handle::query_save_path
                                     | libtorrent::torrent_handle::query_name);

        if (!status.has_metadata) {
            mLogger->warn("operation=check_available_space, message='No metadata available', infoHash={}", mInfoHash);
            return;
        }

        auto spaceInfo = std::experimental::filesystem::space(service->mSettings.download_path);
        if (spaceInfo.free < status.total - status.total_done) {
            mLogger->warn("operation=check_available_space, message='Insufficient space on {}', infoHash={}",
                          status.save_path, mInfoHash);
            pause();
        }
    }

    void Torrent::check_save_resume_data() {
        mLogger->trace("operation=check_save_resume_data, infoHash={}", mInfoHash);
        if (mHandle.is_valid() && mHasMetadata.load() && mHandle.need_save_resume_data()) {
            mHandle.save_resume_data(libtorrent::torrent_handle::save_info_dict);
        }
    }

}
