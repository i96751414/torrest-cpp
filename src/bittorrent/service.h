#ifndef TORREST_SERVICE_H
#define TORREST_SERVICE_H

#include <regex>
#include <mutex>
#include <vector>
#include <atomic>

#include "spdlog/spdlog.h"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/session.hpp"

#include "settings/settings.h"
#include "fwd.h"

namespace torrest {

    class Service {
        friend class Torrent;

    public:
        explicit Service(const Settings &pSettings);

        ~Service();

        void reconfigure(const Settings &pSettings, bool pReset);

    private:
        void consume_alerts_handler();

        void handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert);

        void handle_metadata_received(const libtorrent::metadata_received_alert *pAlert);

        void handle_state_changed(const libtorrent::state_changed_alert *pAlert);

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
        const std::regex mIpRegex = std::regex("\\.\\d+");
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<spdlog::logger> mAlertsLogger;
        Settings mSettings;
        libtorrent::settings_pack mSettingsPack;
        std::shared_ptr<libtorrent::session> mSession;
        std::vector<std::shared_ptr<Torrent>> mTorrents;
        std::mutex mMutex;
        std::vector<std::thread> mThreads;
        std::atomic<bool> mIsRunning;
    };

}

#endif //TORREST_SERVICE_H
