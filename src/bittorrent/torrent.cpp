#include "torrent.h"

namespace torrest {

    Torrent::Torrent(Service *pService, libtorrent::torrent_handle pHandle)
            : mService(pService),
              mHandle(std::move(pHandle)) {}

}
