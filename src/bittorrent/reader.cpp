#include "reader.h"

#include <thread>

#include "libtorrent/storage.hpp"

#include "exceptions.h"

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

    void Reader::wait_for_piece(std::int32_t pPiece) const {
        mTorrent->mLogger->trace("operation=wait_for_piece, piece={}, infoHash={}", pPiece, mTorrent->mInfoHash);
        libtorrent::piece_index_t pieceIndex(pPiece);
        auto startTime = std::chrono::steady_clock::now();

        while (!mTorrent->mHandle.have_piece(pieceIndex)) {
            if (mTorrent->mClosed.load()) {
                throw ReaderException("Torrent closed");
            }
            if (mTorrent->mPaused.load()) {
                throw ReaderException("Torrent paused");
            }
            if (0 < mPieceWaitTimeout && std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - startTime).count() >= mPieceWaitTimeout) {
                mTorrent->mLogger->warn("operation=wait_for_piece, message='Timed out', piece={}, infoHash={}",
                                        pPiece, mTorrent->mInfoHash);
                throw ReaderException("Timeout reached");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    int Reader::read(void *pBuf, std::ptrdiff_t pSize) {
        std::lock_guard<std::mutex> lock(mMutex);
        mTorrent->mLogger->trace("operation=read, pos={}, size={}, infoHash={}", mPos, pSize, mTorrent->mInfoHash);

        auto size = std::min<std::int64_t>(pSize, mSize - mPos);
        if (size == 0) {
            return 0;
        }

        auto startPiece = piece_from_offset(mPos);
        auto endPiece = piece_from_offset(mPos + size - 1);
        set_pieces_priorities(startPiece, endPiece - startPiece);
        for (auto p = startPiece; p <= endPiece; p++) {
            wait_for_piece(p);
        }

        libtorrent::storage_error storageError;
        libtorrent::iovec_t buf{static_cast<char *>(pBuf), static_cast<std::ptrdiff_t>(size)};
        int n = 0;

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
        mTorrent->mLogger->trace("operation=seek, off={}, whence={}, infoHash={}", pOff, pWhence, mTorrent->mInfoHash);
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
            return -1;
        }

        mPos = off;
        set_pieces_priorities(piece_from_offset(off), 0);
        return off;
    }

}}
