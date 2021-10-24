#ifndef TORREST_FILESYSTEM_H
#define TORREST_FILESYSTEM_H

#include <string>

#include "boost/filesystem.hpp"

namespace torrest {

    template<typename... Args>
    boost::filesystem::path join_path(const boost::filesystem::path &pBasePath, Args const &... pArgs) {
        boost::filesystem::path path(pBasePath);
        using unpack = int[];
        (void) unpack{0, (path /= pArgs, 0)...};
        return path;
    }

}

#endif //TORREST_FILESYSTEM_H
