#include "file.h"

#include "torrent.h"
#include "exceptions.h"

#define CHECK_TORRENT(t) if (!t) { throw torrest::bittorrent::InvalidTorrentException("Invalid torrent"); }

namespace torrest { namespace bittorrent {

    File::File(const std::shared_ptr<Torrent> &pTorrent,
               const libtorrent::file_storage &pFileStorage,
               libtorrent::file_index_t pIndex)
            : mTorrent(pTorrent),
              mLogger(pTorrent->mLogger),
              mIndex(pIndex),
              mOffset(pFileStorage.file_offset(pIndex)),
              mSize(pFileStorage.file_size(pIndex)),
              mPath(pFileStorage.file_path(pIndex)),
              mName(pFileStorage.file_name(pIndex)),
              mPieceLength(pFileStorage.piece_length()),
              mPriority(pTorrent->mHandle.file_priority(pIndex)),
              mBuffering(false),
              mBufferSize(0) {
        if (mPriority.load() == libtorrent::dont_download) {
            // Make sure we don't have individual pieces downloading
            // previously set by Buffer
            set_priority(libtorrent::dont_download);
        }
    }

    FileInfo File::get_info() {
        mLogger->trace("operation=get_info");
        return FileInfo{
                .id=int(mIndex),
                .length=mSize,
                .path=mPath,
                .name=mName,
        };
    }

    FileStatus File::get_status() {
        mLogger->trace("operation=get_status");
        std::lock_guard<std::mutex> lock(mMutex);
        auto completed = get_completed();
        return FileStatus{
                .total=mSize,
                .total_done=completed,
                .progress=get_progress(completed),
                .priority=std::uint8_t(mPriority.load()),
                .buffering_total=mBufferSize,
                .buffering_progress=get_buffering_progress(),
                .state=get_state(completed),
        };
    }

    void File::set_priority(libtorrent::download_priority_t pPriority) {
        auto torrent = mTorrent.lock();
        CHECK_TORRENT(torrent)

        mLogger->debug("operation=set_priority, message='Setting file priority', priority={}, infoHash={}, index={}",
                       to_string(pPriority), torrent->mInfoHash, to_string(mIndex));
        std::lock_guard<std::mutex> lock(mMutex);

        mPriority = pPriority;
        mBuffering = false;
        mBufferSize = 0;
        mBufferPieces.clear();
        torrent->mHandle.file_priority(mIndex, pPriority);
    }

    std::int64_t File::get_completed() {
        mLogger->trace("operation=get_completed");
        auto torrent = mTorrent.lock();
        CHECK_TORRENT(torrent)

        std::vector<std::int64_t> file_progress;
        torrent->mHandle.file_progress(file_progress, libtorrent::torrent_handle::piece_granularity);
        return file_progress.at(int(mIndex));
    }

    double File::get_progress(std::int64_t pCompleted) {
        mLogger->trace("operation=get_progress, completed={}", pCompleted);
        return 100.0 * static_cast<double>(pCompleted) / static_cast<double>(mSize);
    }

    State File::get_state(std::int64_t pCompleted) {
        mLogger->trace("operation=get_state, completed={}", pCompleted);
        auto torrent = mTorrent.lock();
        CHECK_TORRENT(torrent)

        auto state = torrent->get_state();
        if (state == downloading) {
            if (mBuffering.load()) {
                state = buffering;
            } else if (mPriority.load() == libtorrent::dont_download || pCompleted == mSize) {
                state = finished;
            }
        }

        return state;
    }

    std::int64_t File::get_buffer_bytes_missing() {
        mLogger->trace("operation=get_buffer_bytes_missing");
        auto torrent = mTorrent.lock();
        CHECK_TORRENT(torrent)
        return torrent->get_bytes_missing(mBufferPieces);
    }

    std::int64_t File::get_buffer_bytes_completed() {
        mLogger->trace("operation=get_buffer_bytes_completed");
        return mBufferSize - get_buffer_bytes_missing();
    }

    double File::get_buffering_progress() {
        mLogger->trace("operation=get_buffering_progress");
        return mBufferSize == 0 || !mBuffering.load()
               ? 100 : 100 * static_cast<double>(get_buffer_bytes_completed()) / static_cast<double>(mBufferSize);
    }

    bool File::verify_buffering_state() {
        bool buffering = false;

        if (mBuffering.load()) {
            std::lock_guard<std::mutex> lock(mMutex);
            if (mBuffering.load()) {
                if (get_buffer_bytes_missing() == 0) {
                    mBuffering = false;
                } else {
                    buffering = true;
                }
            }
        }

        return buffering;
    }

    std::pair<libtorrent::piece_index_t, libtorrent::piece_index_t>
    File::get_pieces_indexes(std::int64_t pOffset, std::int64_t pLength) const {
        mLogger->trace("operation=get_pieces_indexes, index={}, offset={}, length={}",
                       to_string(mIndex), pOffset, pLength);
        return std::pair<libtorrent::piece_index_t, libtorrent::piece_index_t>(
                (mOffset + pOffset) / mPieceLength, (mOffset + pOffset + pLength - 1) / mPieceLength);
    }

    void File::add_buffer_pieces(std::int64_t pOffset, std::int64_t pLength) {
        mLogger->trace("operation=add_buffer_pieces, index={}, offset={}, length={}",
                       to_string(mIndex), pOffset, pLength);
        auto torrent = mTorrent.lock();
        CHECK_TORRENT(torrent)
        auto torrent_file = torrent->mHandle.torrent_file();
        auto pieces = get_pieces_indexes(pOffset, pLength);

        for (auto piece = pieces.first; piece <= pieces.second; piece++) {
            torrent->mHandle.piece_priority(piece, libtorrent::top_priority);
            torrent->mHandle.set_piece_deadline(piece, 0);
            mBufferSize += torrent_file->piece_size(piece);
            mBufferPieces.push_back(piece);
        }
    }

    void File::buffer(std::int64_t pStartBufferSize, std::int64_t pEndBufferSize) {
        mLogger->debug("operation=buffer, index={}, startBufferSize={}, endBufferSize={}",
                       to_string(mIndex), pStartBufferSize, pEndBufferSize);

        auto start_buffer_size = std::max(pStartBufferSize, 0L);
        auto end_buffer_size = std::max(pEndBufferSize, 0L);

        std::lock_guard<std::mutex> lock(mMutex);

        mBufferSize = 0;
        mBufferPieces.clear();

        if (mSize > start_buffer_size + end_buffer_size) {
            add_buffer_pieces(0, start_buffer_size);
            add_buffer_pieces(mSize - end_buffer_size, end_buffer_size);
        } else {
            add_buffer_pieces(0, mSize);
        }

        mBuffering = mBufferSize > 0;
    }

}}
