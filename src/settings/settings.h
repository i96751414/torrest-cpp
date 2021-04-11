#ifndef TORREST_SETTINGS_H
#define TORREST_SETTINGS_H

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace torrest {

    struct Settings {
        unsigned int listen_port = 6889;
        spdlog::level::level_enum alert_log_level = spdlog::level::critical;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings, listen_port, alert_log_level)

        static Settings load(const std::string &path);

        static Settings parse(const std::string &json);

        void save(const std::string &path);

        std::string dump();

        void validate() const;
    };
}

#endif //TORREST_SETTINGS_H
