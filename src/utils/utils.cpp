#include "utils.h"

#include <stdexcept>

namespace torrest { namespace utils {

    std::string sanitize_ip_address(const std::string &pIp, const int &pLeadingGroups) {
        std::string out;
        std::size_t prev = 0;
        std::size_t pos;
        int count = 0;

        while ((pos = pIp.find_first_of(":.", prev)) != std::string::npos) {
            if (pos > prev) {
                out += count++ < pLeadingGroups ? pIp.substr(prev, pos - prev) : std::string(pos - prev, 'X');
            }
            out += pIp[pos];
            prev = pos + 1;
        }

        if (prev < pIp.length()) {
            out += count < pLeadingGroups ? pIp.substr(prev, std::string::npos) : std::string(pIp.length() - prev, 'X');
        }

        return out;
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