#include "reader.h"

#include <thread>

#ifdef TORREST_LEGACY_READ_PIECE
#include "libtorrent/storage.hpp"
#include "exceptions.h"
#endif

namespace torrest { namespace bittorrent {

    Reader::Reader(std::shared_ptr<Torrent> pTorrent,
                   std::int64_t pOffset,
                   std::int64_t pSize,
                   std::int64_t pPieceLength,
                   double pReadAhead,
                   int pPieceWaitTimeout)
            : mTorrent(std::move(pTorrent)),
              mOffset(pOffset),
              mSize(pSize),
              mPieceLength(pPieceLength),
              mPPieces(std::lround(pReadAhead * static_cast<double>(pSize) / static_cast<double>(pPieceLength))),
              mPieceWaitTimeout(pPieceWaitTimeout),
              mLastPiece(piece_from_offset(pSize - 1)),
              mPos(0) {}

    std::int32_t Reader::piece_from_offset(std::int64_t pOffset) const {
        return std::int32_t((mOffset + pOffset) / mPieceLength);
    }

    std::int32_t Reader::piece_offset_from_offset(std::int64_t pOffset) const {
        return std::int32_t((mOffset + pOffset) % mPieceLength);
    }

    std::int64_t Reader::read(void *pBuf, std::int64_t pSize) {
        std::lock_guard<std::mutex> lock(mMutex);
        mTorrent->mLogger->trace("operation=read, pos={}, size={}, infoHash={}", mPos, pSize, mTorrent->mInfoHash);

        auto size = std::min<std::int64_t>(pSize, mSize - mPos);
        if (size <= 0) {
            mTorrent->mLogger->debug("operation=read, message='Nothing to read', requestedSize={}, size={}",
                                     pSize, size);
            return 0;
        }

        auto startPiece = piece_from_offset(mPos);
        auto endPiece = piece_from_offset(mPos + size - 1);
        set_pieces_priorities(startPiece, endPiece - startPiece);
        for (auto p = startPiece; p <= endPiece; p++) {
            mTorrent->wait_for_piece(libtorrent::piece_index_t(p), mPieceWaitTimeout);
        }

#ifdef TORREST_LEGACY_READ_PIECE
        libtorrent::storage_error storageError;
        libtorrent::iovec_t buf{static_cast<char *>(pBuf), static_cast<std::ptrdiff_t>(size)};
        std::int64_t n = 0;

        while (n != size) {
            auto pos = mPos + n;
            auto readSize = mTorrent->mHandle.get_storage_impl()->readv(
                    buf, libtorrent::piece_index_t(piece_from_offset(pos)), piece_offset_from_offset(pos),
                    libtorrent::open_mode::read_only, storageError);

            if (storageError.ec.failed()) {
                mTorrent->mLogger->error("operation=read, message='{}', infoHash={}",
                                         storageError.ec.message(), mTorrent->mInfoHash);
                throw ReaderException("Read failed");
            }
            if (readSize == 0) {
                throw ReaderException("No data to read");
            }

            buf = buf.subspan(readSize);
            n += readSize;
        }
#else
        auto startPieceData = mTorrent->read_piece(
                libtorrent::piece_index_t(startPiece), std::chrono::milliseconds(2000));
        auto startPieceOffset = piece_offset_from_offset(mPos);
        auto n = std::min<std::int64_t>(size, startPieceData.size - startPieceOffset);
        memcpy(pBuf, &startPieceData.buffer[startPieceOffset], n);

        for (auto p = startPiece + 1; p <= endPiece; p++) {
            auto pieceData = mTorrent->read_piece(
                    libtorrent::piece_index_t(p), std::chrono::milliseconds(2000));
            auto pieceBufferSize = std::min<std::int64_t>(size - n, pieceData.size);
            memcpy((char *) pBuf + n, pieceData.buffer.get(), pieceBufferSize);
            n += pieceBufferSize;
        }
#endif // TORREST_LEGACY_READ_PIECE

        mPos += n;
        return n;
    }

    void Reader::set_piece_priority(libtorrent::piece_index_t pPiece, int pDeadline,
                                    libtorrent::download_priority_t pPriority) const {
        if (mTorrent->mHandle.piece_priority(pPiece) < pPriority) {
            mTorrent->mHandle.piece_priority(pPiece, pPriority);
            mTorrent->mHandle.set_piece_deadline(pPiece, pDeadline);
        }
    }

    void Reader::set_pieces_priorities(std::int32_t pPiece, std::int32_t pPieceEndOffset) const {
        auto endPiece = pPiece + pPieceEndOffset + mPPieces;
        for (std::int32_t i = 0, p = pPiece; p <= endPiece && p <= mLastPiece; p++, i++) {
            libtorrent::piece_index_t pieceIndex(p);
            if (!mTorrent->mHandle.have_piece(pieceIndex)) {
                if (i <= pPieceEndOffset) {
                    set_piece_priority(pieceIndex, 0, libtorrent::top_priority);
                } else {
                    set_piece_priority(pieceIndex, (i - pPieceEndOffset) * 10, libtorrent::download_priority_t(6));
                }
            }
        }
    }

    std::int64_t Reader::seek(std::int64_t pOff, int pWhence) {
        mTorrent->mLogger->debug("operation=seek, off={}, whence={}, infoHash={}", pOff, pWhence, mTorrent->mInfoHash);
        std::lock_guard<std::mutex> lock(mMutex);

        std::int64_t off;
        switch (pWhence) {
            case std::ios::beg:
                off = pOff;
                break;
            case std::ios::cur:
                off = mPos + pOff;
                break;
            case std::ios::end:
                off = mSize + pOff;
                break;
            default:
                off = -1;
        }

        if (off < 0 || off > mSize) {
            mTorrent->mLogger->error("operation=seek, message='Offset out of boundaries', off={}", off);
            return -1;
        }

        mPos = off;
        set_pieces_priorities(piece_from_offset(off), 0);
        return off;
    }

}}
