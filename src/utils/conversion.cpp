#include "conversion.h"

#include <stdexcept>
#include <sstream>

namespace torrest {
    uint16_t str_to_uint16(const char *pStr) {
        int value(std::stoi(pStr));
        if (value < 0 || value > static_cast<int>(UINT16_MAX)) {
            throw std::runtime_error("Invalid uint16 value");
        }
        return static_cast<uint16_t>(value);
    }

    std::string join_string_vector(const std::vector<std::string> &pVector, const std::string &pDelimiter) {
        std::ostringstream s;
        auto it = pVector.begin();

        if (it != pVector.end()) {
            s << *it++;

            while (it != pVector.end()) {
                s << pDelimiter << *it++;
            }
        }

        return s.str();
    }
}