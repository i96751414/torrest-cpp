#include "torrent.h"

#include <experimental/filesystem>
#include <utility>

#include "libtorrent/torrent_status.hpp"
#include "libtorrent/torrent_info.hpp"
#include "service.h"
#include "file.h"
#include "exceptions.h"

namespace torrest { namespace bittorrent {

    Torrent::Torrent(libtorrent::torrent_handle pHandle,
                     std::string pInfoHash,
                     std::shared_ptr<spdlog::logger> pLogger)
            : mLogger(std::move(pLogger)),
              mHandle(std::move(pHandle)),
              mInfoHash(std::move(pInfoHash)),
              mHasMetadata(false),
              mClosed(false) {

        auto flags = mHandle.flags();
        auto status = mHandle.status(libtorrent::torrent_handle::query_name);

        mPaused = (flags & libtorrent::torrent_flags::paused) && !(flags & libtorrent::torrent_flags::auto_managed);
        mDefaultName = status.name.empty() ? mInfoHash : status.name;
    }

    void Torrent::handle_metadata_received() {
        mLogger->debug("operation=handle_metadata_received");
        std::lock_guard<std::mutex> lock(mFilesMutex);
        auto torrentFile = mHandle.torrent_file();
        auto files = torrentFile->files();

        mFiles.clear();
        for (int i = 0; i < torrentFile->num_files(); i++) {
            mFiles.emplace_back(std::make_shared<File>(shared_from_this(), files, libtorrent::file_index_t(i)));
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

    void Torrent::set_priority(libtorrent::download_priority_t pPriority) {
        mLogger->debug("operation=set_priority, priority={}", to_string(pPriority));
        if (!mHasMetadata.load()) {
            throw NoMetadataException("No metadata");
        }
        std::lock_guard<std::mutex> lock(mFilesMutex);
        for (auto &file : mFiles) {
            file->set_priority(pPriority);
        }
    }

    TorrentInfo Torrent::get_info() {
        mLogger->trace("operation=get_info");
        auto torrentFile = mHandle.torrent_file();
        return TorrentInfo{
                .info_hash=mInfoHash,
                .name=torrentFile ? torrentFile->name() : mDefaultName,
                .size=torrentFile ? torrentFile->total_size() : 0,
        };
    }

    TorrentStatus Torrent::get_status() {
        mLogger->trace("operation=get_status");
        std::lock_guard<std::mutex> lock(mMutex);
        auto status = mHandle.status();
        auto peers = status.num_peers - status.num_seeds;

        return TorrentStatus{
                .total=status.total,
                .total_done=status.total_done,
                .total_wanted=status.total_wanted,
                .total_wanted_done=status.total_wanted_done,
                .progress=status.progress * 100,
                .download_rate=status.download_rate,
                .upload_rate=status.upload_rate,
                .paused=mPaused.load(),
                .has_metadata=mHasMetadata.load(),
                .state=get_state(),
                .seeders=status.num_seeds,
                .seeders_total=(status.num_complete < 0 ? status.num_seeds : status.num_complete),
                .peers=peers,
                .peers_total=(status.num_incomplete < 0 ? peers : status.num_incomplete),
                .seeding_time=status.seeding_duration.count(),
                .finished_time=status.finished_duration.count(),
                .active_time=status.active_duration.count(),
                .all_time_download=status.all_time_download,
                .all_time_upload=status.all_time_upload,
        };
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

    std::vector<std::shared_ptr<File>> Torrent::get_files() {
        mLogger->trace("operation=get_files");
        if (!mHasMetadata.load()) {
            throw NoMetadataException("No metadata");
        }
        std::lock_guard<std::mutex> lock(mFilesMutex);
        return mFiles;
    }

    void Torrent::check_available_space(const std::string &pPath) {
        mLogger->debug("operation=check_available_space, message='Checking available space', infoHash={}", mInfoHash);

        auto status = mHandle.status(libtorrent::torrent_handle::query_accurate_download_counters
                                     | libtorrent::torrent_handle::query_save_path
                                     | libtorrent::torrent_handle::query_name);

        if (!status.has_metadata) {
            mLogger->warn("operation=check_available_space, message='No metadata available', infoHash={}", mInfoHash);
            return;
        }

        auto spaceInfo = std::experimental::filesystem::space(pPath);
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

    std::int64_t Torrent::get_bytes_missing(const std::vector<libtorrent::piece_index_t> &pPieces) {
        mLogger->debug("operation=get_bytes_missing");
        auto torrentFile = mHandle.torrent_file();
        std::int64_t missing = 0;

        for (auto &piece : pPieces) {
            if (!mHandle.have_piece(piece)) {
                missing += torrentFile->piece_size(piece);
            }
        }

        if (missing > 0) {
            std::vector<libtorrent::partial_piece_info> queue;
            mHandle.get_download_queue(queue);

            for (auto &qPiece: queue) {
                if (std::find(pPieces.begin(), pPieces.end(), qPiece.piece_index) != pPieces.end()) {
                    for (int i = 0; i < qPiece.blocks_in_piece; i++) {
                        missing -= qPiece.blocks[i].bytes_progress;
                    }
                }
            }
        }

        return missing;
    }

}}
