#ifndef TORREST_SERVICE_H
#define TORREST_SERVICE_H

#include "spdlog/spdlog.h"
#include "libtorrent/settings_pack.hpp"

#include "settings/settings.h"

namespace torrest {

    class Service {
    public:
        explicit Service(const Settings &pSettings);

    private:
        std::shared_ptr<spdlog::logger> mLogger;
        Settings mSettings;
        libtorrent::settings_pack mSettingsPack;
    };

}

#endif //TORREST_SERVICE_H
