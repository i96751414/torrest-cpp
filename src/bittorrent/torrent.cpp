#include "torrent.h"

#include "boost/filesystem.hpp"
#include "libtorrent/torrent_status.hpp"
#include "libtorrent/torrent_info.hpp"
#include "service.h"
#include "file.h"
#include "exceptions.h"

namespace torrest { namespace bittorrent {

    Torrent::Torrent(std::shared_ptr<ServiceSettings> pSettings,
                     libtorrent::torrent_handle pHandle,
                     std::string pInfoHash,
                     std::shared_ptr<spdlog::logger> pLogger)
            : mLogger(std::move(pLogger)),
              mHandle(std::move(pHandle)),
              mSettings(std::move(pSettings)),
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

    void Torrent::store_piece(libtorrent::piece_index_t pPiece, int pSize, const boost::shared_array<char> &pBuffer) {
        mLogger->trace("operation=store_piece, piece={}, size={}", to_string(pPiece), pSize);
        std::lock_guard<std::mutex> lock(mPiecesMutex);
        mPieces[pPiece] = PieceData{.size=pSize, .buffer=pBuffer, .read_at=std::chrono::steady_clock::now()};
        mPiecesCv.notify_all();
    }

    void Torrent::cleanup_pieces(const std::chrono::milliseconds &pExpiration) {
        mLogger->trace("operation=cleanup_pieces, expiration={}", pExpiration.count());
        auto now = std::chrono::steady_clock::now();
        std::lock_guard<std::mutex> lock(mPiecesMutex);
        for (auto it = mPieces.begin(); it != mPieces.end();) {
            if (now - it->second.read_at >= pExpiration) {
                mPieces.erase(it++);
            } else {
                it++;
            }
        }
    }

    void Torrent::schedule_read_piece(libtorrent::piece_index_t pPiece) {
        mLogger->trace("operation=schedule_read_piece, piece={}", to_string(pPiece));
        mHandle.read_piece(pPiece);
    }

    PieceData Torrent::read_scheduled_piece(libtorrent::piece_index_t pPiece,
                                            const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pWaitUntil) {
        mLogger->trace("operation=read_scheduled_piece, piece={}, withTimeout={}",
                       to_string(pPiece), pWaitUntil.has_value());
        std::unique_lock<std::mutex> lock(mPiecesMutex);

        auto it = mPieces.find(pPiece);
        while (it == mPieces.end()) {
            if (!pWaitUntil) {
                mPiecesCv.wait(lock);
            } else if (std::chrono::steady_clock::now() >= *pWaitUntil) {
                mLogger->error("operation=read_scheduled_piece, message='Timed out waiting for piece', piece={}",
                               to_string(pPiece));
                throw PieceException("Timed out waiting for piece");
            } else {
                mPiecesCv.wait_until(lock, *pWaitUntil);
            }

            it = mPieces.find(pPiece);
        }

        it->second.read_at = std::chrono::steady_clock::now();
        return it->second;
    }

    PieceData Torrent::read_piece(libtorrent::piece_index_t pPiece,
                                  const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pWaitUntil) {
        mLogger->trace("operation=read_piece, piece={}, withTimeout={}", to_string(pPiece), pWaitUntil.has_value());

        {
            std::lock_guard<std::mutex> lock(mPiecesMutex);
            auto it = mPieces.find(pPiece);
            if (it != mPieces.end()) {
                it->second.read_at = std::chrono::steady_clock::now();
                return it->second;
            }
        }

        schedule_read_piece(pPiece);
        return read_scheduled_piece(pPiece, pWaitUntil);
    }

    void Torrent::wait_for_piece(libtorrent::piece_index_t pPiece,
                                 const boost::optional<std::chrono::time_point<std::chrono::steady_clock>> &pUntil) const {
        mLogger->trace("operation=wait_for_piece, piece={}, infoHash={}", to_string(pPiece), mInfoHash);
        auto startTime = std::chrono::steady_clock::now();
        std::chrono::time_point<std::chrono::steady_clock> time = std::chrono::steady_clock::now();

        while (!mHandle.have_piece(pPiece)) {
            if (mClosed.load()) {
                throw PieceException("Torrent closed");
            }
            if (mPaused.load()) {
                throw PieceException("Torrent paused");
            }
            if (pUntil && std::chrono::steady_clock::now() >= *pUntil) {
                mLogger->warn("operation=wait_for_piece, message='Timed out', piece={}, infoHash={}",
                              to_string(pPiece), mInfoHash);
                throw PieceException("Timeout reached");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
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

    TorrentInfo Torrent::get_info() const {
        mLogger->trace("operation=get_info");
        auto torrentFile = mHandle.torrent_file();
        return TorrentInfo{
                .info_hash=mInfoHash,
                .name=torrentFile ? torrentFile->name() : mDefaultName,
                .size=torrentFile ? torrentFile->total_size() : 0,
        };
    }

    TorrentStatus Torrent::get_status() const {
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

    State Torrent::get_state() const {
        mLogger->trace("operation=get_state");
        auto state = get_torrent_state();

#ifdef TORREST_ENABLE_TORRENT_BUFFERING_STATUS
        if (state == downloading) {
            std::lock_guard<std::mutex> filesLock(mFilesMutex);
            for (auto &file : mFiles) {
                if (file->mBuffering.load()) {
                    state = buffering;
                    break;
                }
            }
        }
#endif

        return state;
    }

    State Torrent::get_torrent_state() const {
        mLogger->trace("operation=get_torrent_state");
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
                mLogger->warn(
                        "operation=get_torrent_state, message='Unknown torrent state', torrentState={}",
                        torrentState);
        }

        return state;
    }

    std::vector<std::shared_ptr<File>> Torrent::get_files() const {
        mLogger->trace("operation=get_files");
        if (!mHasMetadata.load()) {
            throw NoMetadataException("No metadata");
        }
        std::lock_guard<std::mutex> lock(mFilesMutex);
        return mFiles;
    }

    std::shared_ptr<File> Torrent::get_file(int pIndex) const {
        mLogger->trace("operation=get_file, index={}", pIndex);
        if (!mHasMetadata.load()) {
            throw NoMetadataException("No metadata");
        }
        std::lock_guard<std::mutex> lock(mFilesMutex);
        if (pIndex < 0 || pIndex >= mFiles.size()) {
            throw InvalidFileIndexException("No such file index");
        }
        return mFiles.at(pIndex);
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

        auto spaceInfo = boost::filesystem::space(pPath);
        if (spaceInfo.free < status.total - status.total_done) {
            mLogger->warn("operation=check_available_space, message='Insufficient space on {}', infoHash={}",
                          status.save_path, mInfoHash);
            pause();
        }
    }

    void Torrent::check_save_resume_data() const {
        mLogger->trace("operation=check_save_resume_data, infoHash={}", mInfoHash);
        if (mHandle.is_valid() && mHasMetadata.load() && mHandle.need_save_resume_data()) {
            mHandle.save_resume_data(libtorrent::torrent_handle::save_info_dict);
        }
    }

    std::int64_t Torrent::get_bytes_missing(const std::vector<libtorrent::piece_index_t> &pPieces) const {
        mLogger->trace("operation=get_bytes_missing");
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

            for (auto &qPiece : queue) {
                if (std::find(pPieces.begin(), pPieces.end(), qPiece.piece_index) != pPieces.end()) {
                    for (int i = 0; i < qPiece.blocks_in_piece; i++) {
                        missing -= qPiece.blocks[i].bytes_progress;
                    }
                }
            }
        }

        return missing;
    }

    bool Torrent::verify_buffering_state() const {
        std::lock_guard<std::mutex> lock(mFilesMutex);
        bool has_files_buffering = false;

        for (auto &file : mFiles) {
            if (file->verify_buffering_state()) {
                has_files_buffering = true;
            }
        }

        return has_files_buffering;
    }

}}
