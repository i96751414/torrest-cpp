#ifndef TORREST_CONVERSION_H
#define TORREST_CONVERSION_H

#include <string>

namespace torrest {
    uint16_t str_to_uint16(const char *str) {
        int value(std::stoi(str));
        if (value < 0 || value > static_cast<int>(UINT16_MAX)) {
            throw std::runtime_error("Invalid uint16 value");
        }
        return static_cast<uint16_t>(value);
    }
}

#endif //TORREST_CONVERSION_H
