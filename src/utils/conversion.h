#ifndef TORREST_CONVERSION_H
#define TORREST_CONVERSION_H

#include <string>
#include <vector>

namespace torrest {
    uint16_t str_to_uint16(const char *pStr);

    std::string join_string_vector(const std::vector<std::string> &pVector, const std::string &pDelimiter);

    std::string unescape_string(const std::string &pStr);

    inline std::string &ltrim(std::string &pStr, const char *pChars = " \t\n\r\f\v") {
        pStr.erase(0, pStr.find_first_not_of(pChars));
        return pStr;
    }

    inline std::string ltrim_copy(std::string pStr, const char *pChars = " \t\n\r\f\v") {
        return ltrim(pStr, pChars);
    }
}

#endif //TORREST_CONVERSION_H
