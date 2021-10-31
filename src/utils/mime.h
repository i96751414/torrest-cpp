#ifndef TORREST_MIME_H
#define TORREST_MIME_H

#include <string>

namespace torrest { namespace utils {

    std::string guess_mime_type(const std::string &pExtension);

}}

#endif //TORREST_MIME_H
