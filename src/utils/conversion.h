#ifndef TORREST_CONVERSION_H
#define TORREST_CONVERSION_H

#include <string>
#include <vector>

namespace torrest {
    uint16_t str_to_uint16(const char *pStr);

    std::string join_string_vector(const std::vector<std::string> &pVector, const std::string &pDelimiter);
}

#endif //TORREST_CONVERSION_H
