#ifndef TORREST_SERVICE_H
#define TORREST_SERVICE_H

#include <regex>
#include <mutex>
#include <vector>
#include <atomic>
#include <condition_variable>
#include <memory>

#include "spdlog/spdlog.h"
#include "libtorrent/settings_pack.hpp"
#include "libtorrent/session.hpp"

#include "settings/settings.h"
#include "torrent.h"

namespace torrest {

    class Service : std::enable_shared_from_this<Service> {
        friend class Torrent;

    public:
        explicit Service(const Settings &pSettings);

        ~Service();

        void reconfigure(const Settings &pSettings, bool pReset);

        std::shared_ptr<Torrent> get_torrent(const std::string &pInfoHash);

        void remove_torrent(const std::string &pInfoHash, bool pRemoveFiles);

        std::string add_magnet(const std::string &pMagnet, bool pDownload);

        std::string add_torrent_data(const std::vector<char> &pData, bool pDownload);

        std::string add_torrent_file(const std::string &pFile, bool pDownload);

    private:
        void check_save_resume_data_handler();

        void consume_alerts_handler();

        void progress_handler();

        void handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert);

        void handle_metadata_received(const libtorrent::metadata_received_alert *pAlert);

        void handle_state_changed(const libtorrent::state_changed_alert *pAlert);

        void configure(const Settings &pSettings);

        void set_buffering_rate_limits(bool pEnable);

        void remove_torrents();

        void add_torrent_with_params(libtorrent::add_torrent_params &pTorrentParams,
                                     const std::string &pInfoHash,
                                     bool pIsResumeData,
                                     bool pDownload);

        std::string add_magnet(const std::string &pMagnet, bool pDownload, bool pSaveMagnet);

        void add_torrent_with_resume_data(const std::string &pFile);

        void load_torrent_files();

        std::vector<std::shared_ptr<Torrent>>::iterator find_torrent(const std::string &pInfoHash,
                                                                     bool pMustFind = true);

        bool has_torrent(const std::string &pInfoHash);

        std::string get_parts_file(const std::string &pInfoHash) const;

        std::string get_fast_resume_file(const std::string &pInfoHash) const;

        std::string get_torrent_file(const std::string &pInfoHash) const;

        std::string get_magnet_file(const std::string &pInfoHash) const;

        void delete_parts_file(const std::string &pInfoHash) const;

        void delete_fast_resume_file(const std::string &pInfoHash) const;

        void delete_torrent_file(const std::string &pInfoHash) const;

        void delete_magnet_file(const std::string &pInfoHash) const;

        bool wait_for_abort(const int &pSeconds);

        bool wait_for_abort(const std::chrono::seconds &pSeconds);

        const std::regex mPortRegex = std::regex(":\\d+$");
        const std::regex mWhiteSpaceRegex = std::regex("\\s+");
        const std::regex mIpRegex = std::regex("\\.\\d+");
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<spdlog::logger> mAlertsLogger;
        Settings mSettings;
        libtorrent::settings_pack mSettingsPack;
        std::shared_ptr<libtorrent::session> mSession;
        std::vector<std::shared_ptr<Torrent>> mTorrents;
        std::mutex mTorrentsMutex;
        std::mutex mServiceMutex;
        std::mutex mCvMutex;
        std::condition_variable mCv;
        std::vector<std::thread> mThreads;
        std::atomic<bool> mIsRunning;
    };

}

#endif //TORREST_SERVICE_H
