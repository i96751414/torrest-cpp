#include "file.h"

#include "torrent.h"

namespace torrest {

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
              mBuffering(false) {
        if (mPriority.load() == libtorrent::dont_download) {
            // Make sure we don't have individual pieces downloading
            // previously set by Buffer
            set_priority(libtorrent::dont_download);
        }
    }

    void File::set_priority(libtorrent::download_priority_t pPriority) {
        mLogger->debug("operation=set_priority, message='Setting file priority', priority={}, infoHash={}",
                       to_string(pPriority), mTorrent->mInfoHash);
        std::lock_guard<std::mutex> lock(mMutex);

        mPriority = pPriority;
        if (pPriority == libtorrent::dont_download) {
            mBuffering = false;
        }
        mTorrent->mHandle.file_priority(mIndex, pPriority);
    }
}
