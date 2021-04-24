#ifndef TORREST_FILE_H
#define TORREST_FILE_H

#include <mutex>
#include <atomic>

#include "spdlog/spdlog.h"
#include <libtorrent/download_priority.hpp>
#include "libtorrent/file_storage.hpp"

#include "fwd.h"

namespace torrest {

    class File {
    public:
        File(Torrent *pTorrent, const libtorrent::file_storage &pFileStorage, libtorrent::file_index_t pIndex);

        void set_priority(libtorrent::download_priority_t pPriority);

    private:
        Torrent *mTorrent;
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
    };

}

#endif //TORREST_FILE_H
