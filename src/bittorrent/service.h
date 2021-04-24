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
#include "torrent.h"

namespace torrest {

    enum PeerTos {
        tos_normal = 0x00,
        tos_min_cost = 0x02,
        tos_max_reliability = 0x04,
        tos_max_throughput = 0x08,
        tos_min_delay = 0x10,
        tos_scavenger = 0x20
    };

    class Service {
        friend class Torrent;

    public:
        explicit Service(const Settings &pSettings);

        ~Service();

        void reconfigure(const Settings &pSettings, bool pReset);

        std::shared_ptr<Torrent> get_torrent(const std::string &pInfoHash);

        void remove_torrent(const std::string &pInfoHash, bool pRemoveFiles);

    private:
        void consume_alerts_handler();

        void handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert);

        void handle_metadata_received(const libtorrent::metadata_received_alert *pAlert);

        void handle_state_changed(const libtorrent::state_changed_alert *pAlert);

        void configure(const Settings &pSettings);

        void set_buffering_rate_limits(bool pEnable);

        std::vector<std::shared_ptr<Torrent>>::iterator find_torrent(const std::string &pInfoHash);

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
