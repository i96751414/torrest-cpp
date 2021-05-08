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
        std::lock_guard<std::mutex> lock(mFilesMutex);
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

    State Torrent::get_state() {
        mLogger->trace("operation=get_state");
        if (mPaused.load()) {
            return paused;
        }
        auto flags = mHandle.flags();
        if (flags & libtorrent::torrent_flags::paused && flags & libtorrent::torrent_flags::auto_managed) {
            return queued;
        }
        if (!mHasMetadata.load()) {
            return finding;
        }

        State state = queued;
        auto torrentState = mHandle.status().state;

        switch (torrentState) {
            case libtorrent::torrent_status::checking_files:
                state = checking;
                break;
            case libtorrent::torrent_status::downloading_metadata:
                state = finding;
                break;
            case libtorrent::torrent_status::downloading:
                state = downloading;
                break;
            case libtorrent::torrent_status::finished:
                state = finished;
                break;
            case libtorrent::torrent_status::seeding:
                state = seeding;
                break;
            case libtorrent::torrent_status::checking_resume_data:
                state = checking_resume_data;
                break;
            case libtorrent::torrent_status::unused_enum_for_backwards_compatibility_allocating:
                state = allocating;
                break;
            default:
                mLogger->warn("operation=get_state, message='Unknown torrent state', torrentState={}", torrentState);
        }

        return state;
    }

    double Torrent::get_files_progress() {
        mLogger->trace("operation=get_files_progress");
        std::lock_guard<std::mutex> lock(mFilesMutex);
        std::vector<std::int64_t> file_progress;
        mHandle.file_progress(file_progress, libtorrent::torrent_handle::piece_granularity);

        std::int64_t total = 0;
        std::int64_t completed = 0;

        for (auto &file : mFiles) {
            total += file->mSize;
            completed += file_progress.at(int(file->mIndex));
        }

        if (total == 0) {
            return 100;
        }

        double progress = 100.0 * static_cast<double>(completed) / static_cast<double>(total);
        return progress > 100 ? 100 : progress;
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
