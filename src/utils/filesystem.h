#ifndef TORREST_FILESYSTEM_H
#define TORREST_FILESYSTEM_H

#include <string>
#include <experimental/filesystem>

namespace torrest {

    template<typename... Args>
    std::experimental::filesystem::path join_path(const std::experimental::filesystem::path &pBasePath,
                                                  Args const &... pArgs) {
        std::experimental::filesystem::path path(pBasePath);
        using unpack = int[];
        (void) unpack{0, (path /= pArgs, 0)...};
        return path;
    }

}

#endif //TORREST_FILESYSTEM_H
