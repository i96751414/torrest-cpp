#ifndef TORREST_SERVICE_H
#define TORREST_SERVICE_H

#include <regex>

#include "spdlog/spdlog.h"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/session.hpp"

#include "settings/settings.h"

namespace torrest {

    class Service {
    public:
        explicit Service(Settings pSettings);

    private:
        void configure();

        void set_buffering_rate_limits(bool enable);

        const std::regex mPortRegex = std::regex(":\\d+$");
        const std::regex mWhiteSpaceRegex = std::regex("\\s+");
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<spdlog::logger> mAlertsLogger;
        Settings mSettings;
        libtorrent::settings_pack mSettingsPack;
        std::shared_ptr<libtorrent::session> mSession;
    };

}

#endif //TORREST_SERVICE_H
