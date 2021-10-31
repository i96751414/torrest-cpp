#include "conversion.h"

#include <stdexcept>
#include <sstream>

namespace torrest { namespace utils {

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

    std::string unescape_string(const std::string &pStr) {
        std::string ret;
        for (auto i = pStr.cbegin(); i != pStr.cend(); ++i) {
            if (*i == '+') {
                ret += ' ';
            } else if (*i != '%') {
                ret += *i;
            } else {
                ++i;
                if (i == pStr.cend()) {
                    throw std::runtime_error("Invalid escaped string");
                }

                int high;
                if (*i >= '0' && *i <= '9') high = *i - '0';
                else if (*i >= 'A' && *i <= 'F') high = *i + 10 - 'A';
                else if (*i >= 'a' && *i <= 'f') high = *i + 10 - 'a';
                else {
                    throw std::runtime_error("Invalid escaped string");
                }

                ++i;
                if (i == pStr.cend()) {
                    throw std::runtime_error("Invalid escaped string");
                }

                int low;
                if (*i >= '0' && *i <= '9') low = *i - '0';
                else if (*i >= 'A' && *i <= 'F') low = *i + 10 - 'A';
                else if (*i >= 'a' && *i <= 'f') low = *i + 10 - 'a';
                else {
                    throw std::runtime_error("Invalid escaped string");
                }

                ret += char(high * 16 + low);
            }
        }
        return ret;
    }

}}