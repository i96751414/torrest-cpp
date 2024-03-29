#ifndef TORREST_READER_H
#define TORREST_READER_H

#include <memory>
#include <mutex>

#include "libtorrent/units.hpp"

#include "torrent.h"

namespace torrest { namespace bittorrent {

    class Reader {
    public:
        Reader(std::shared_ptr<Torrent> pTorrent,
               std::int64_t pOffset,
               std::int64_t pSize,
               std::int64_t pPieceLength,
               double pReadAhead,
               int pPieceWaitTimeout);

        std::int64_t read(void *pBuf, std::int64_t pSize);

        std::int64_t seek(std::int64_t pOff, int pWhence);

    private:
        std::int32_t piece_from_offset(std::int64_t pOffset) const;

        std::int32_t piece_offset_from_offset(std::int64_t pOffset) const;

        void set_piece_priority(libtorrent::piece_index_t pPiece, int pDeadline,
                                libtorrent::download_priority_t pPriority) const;

        void set_pieces_priorities(std::int32_t pPiece, std::int32_t pPieceEndOffset) const;

        mutable std::mutex mMutex;
        std::shared_ptr<Torrent> mTorrent;
        std::int64_t mOffset;
        std::int64_t mSize;
        std::int64_t mPieceLength;
        std::int64_t mPPieces;
        std::chrono::seconds mPieceWaitTimeout;
        std::int32_t mLastPiece;
        std::int64_t mPos;
    };

}}

#endif //TORREST_READER_H
