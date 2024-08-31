#ifndef TORREST_ENUM_FMT_H
#define TORREST_ENUM_FMT_H

#include <type_traits>

namespace enums {
    template<typename T, typename std::enable_if_t<std::is_enum<T>::value, int> = 0>
    constexpr auto format_as(const T &e) {
        return static_cast<typename std::underlying_type<T>::type>(e);
    }
}

namespace torrest { namespace settings {
    using enums::format_as;
}}

namespace libtorrent {
    using enums::format_as;
}

#endif