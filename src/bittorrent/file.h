#ifndef TORREST_FILE_H
#define TORREST_FILE_H

#include <mutex>
#include <atomic>

#include "spdlog/spdlog.h"
#include "libtorrent/download_priority.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/file_storage.hpp"

#include "fwd.h"
#include "enums.h"

namespace torrest { namespace bittorrent {

    class File {
        friend class Service;

        friend class Torrent;

    public:
        File(const std::shared_ptr<Torrent> &pTorrent,
             const libtorrent::file_storage &pFileStorage,
             libtorrent::file_index_t pIndex);

        void set_priority(libtorrent::download_priority_t pPriority);

        std::int64_t get_completed();

        double get_progress();

        State get_state();

        void buffer(std::int64_t pStartBufferSize, std::int64_t pEndBufferSize);

    private:
        std::int64_t get_buffer_bytes_missing();

        std::pair<libtorrent::piece_index_t, libtorrent::piece_index_t>
        get_pieces_indexes(std::int64_t pOffset, std::int64_t pLength) const;

        void add_buffer_pieces(std::int64_t pOffset, std::int64_t pLength);

        bool verify_buffering_state();

        std::weak_ptr<Torrent> mTorrent;
        std::shared_ptr<spdlog::logger> mLogger;
        libtorrent::file_index_t mIndex;
        std::int64_t mOffset;
        std::int64_t mSize;
        std::string mPath;
        std::string mName;
        int mPieceLength;
        std::mutex mMutex;
        std::atomic<libtorrent::download_priority_t> mPriority;
        std::atomic<bool> mBuffering;
        std::vector<libtorrent::piece_index_t> mBufferPieces;
        std::int64_t mBufferSize;
    };

}}

#endif //TORREST_FILE_H
