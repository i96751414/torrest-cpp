#ifndef TORREST_SERVICE_H
#define TORREST_SERVICE_H

#include <regex>
#include <mutex>

#include "spdlog/spdlog.h"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/session.hpp"

#include "settings/settings.h"

namespace torrest {

    class Service {
    public:
        explicit Service(const Settings &pSettings);

        void reconfigure(const Settings &pSettings, bool pReset);

    private:
        void configure(const Settings &pSettings);

        void set_buffering_rate_limits(bool pEnable);

        std::string get_parts_file(const std::string &pInfoHash) const;

        std::string get_fast_resume_file(const std::string &pInfoHash) const;

        std::string get_torrent_file(const std::string &pInfoHash) const;

        std::string get_magnet_file(const std::string &pInfoHash) const;

        void delete_parts_file(const std::string &pInfoHash) const;

        void delete_fast_resume_file(const std::string &pInfoHash) const;

        void delete_torrent_file(const std::string &pInfoHash) const;

        void delete_magnet_file(const std::string &pInfoHash) const;

        const std::regex mPortRegex = std::regex(":\\d+$");
        const std::regex mWhiteSpaceRegex = std::regex("\\s+");
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<spdlog::logger> mAlertsLogger;
        Settings mSettings;
        libtorrent::settings_pack mSettingsPack;
        std::shared_ptr<libtorrent::session> mSession;
        std::mutex mMutex;
    };

}

#endif //TORREST_SERVICE_H
