#ifndef TORREST_TORRENT_H
#define TORREST_TORRENT_H

#include "service.h"

namespace torrest {

    enum Status {
        queued,
        checking,
        finding,
        downloading,
        finished,
        seeding,
        allocating,
        checking_resume_data,
        paused,
        buffering
    };

    class Torrent {
    public:
        Torrent(Service *pService, libtorrent::torrent_handle pHandle);

    private:
        Service *mService;
        libtorrent::torrent_handle mHandle;
        std::vector<std::shared_ptr<File>> mFiles;
    };

}

#endif //TORREST_TORRENT_H
