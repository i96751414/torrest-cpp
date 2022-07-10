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
#include "file.h"
#include "settings.h"

namespace torrest { namespace bittorrent {

    struct ServiceStatus {
        double progress;
        std::int64_t download_rate;
        std::int64_t upload_rate;
        int num_torrents;
        bool paused;
    };

    class Service {
    public:
        explicit Service(const settings::Settings &pSettings);

        ~Service();

        void reconfigure(const settings::Settings &pSettings, bool pReset);

        std::shared_ptr<Torrent> get_torrent(const std::string &pInfoHash) const;

        std::vector<std::shared_ptr<Torrent>> get_torrents() const;

        void remove_torrent(const std::string &pInfoHash, bool pRemoveFiles);

        std::string add_magnet(const std::string &pMagnet, bool pDownload);

        std::string add_torrent_data(const char *pData, int pSize, bool pDownload);

        std::string add_torrent_file(const std::string &pFile, bool pDownload);

        ServiceStatus get_status() const;

        void pause();

        void resume();

    private:
        void check_save_resume_data_handler() const;

        void consume_alerts_handler() const;

        void piece_cleanup_handler() const;

        void progress_handler();

        void update_progress();

        void handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert) const;

        void handle_metadata_received(const libtorrent::metadata_received_alert *pAlert) const;

        void handle_state_changed(const libtorrent::state_changed_alert *pAlert) const;

        void handle_read_piece_alert(const libtorrent::read_piece_alert *pAlert) const;

        void configure(const settings::Settings &pSettings);

        void set_buffering_rate_limits(bool pEnable);

        void remove_torrents();

        void add_torrent_with_params(libtorrent::add_torrent_params &pTorrentParams,
                                     const std::string &pInfoHash,
                                     bool pIsResumeData,
                                     bool pDownload);

        std::string add_torrent_file(const std::string &pFile, bool pDownload, bool pSaveFile);

        std::string add_magnet(const std::string &pMagnet, bool pDownload, bool pSaveMagnet);

        void add_torrent_with_resume_data(const std::string &pFile);

        void load_torrent_files();

        std::vector<std::shared_ptr<Torrent>>::const_iterator find_torrent(const std::string &pInfoHash,
                                                                           bool pMustFind = true) const;

        bool has_torrent(const std::string &pInfoHash) const;

        std::string get_parts_file(const std::string &pInfoHash) const;

        std::string get_fast_resume_file(const std::string &pInfoHash) const;

        std::string get_torrent_file(const std::string &pInfoHash) const;

        std::string get_magnet_file(const std::string &pInfoHash) const;

        void delete_parts_file(const std::string &pInfoHash) const;

        void delete_fast_resume_file(const std::string &pInfoHash) const;

        void delete_torrent_file(const std::string &pInfoHash) const;

        void delete_magnet_file(const std::string &pInfoHash) const;

        bool wait_for_abort(const int &pSeconds) const;

        bool wait_for_abort(const std::chrono::seconds &pSeconds) const;

        const std::regex mPortRegex = std::regex(":\\d+$");
        const std::regex mWhiteSpaceRegex = std::regex("\\s+");
        const std::regex mIpRegex = std::regex("\\.\\d+");
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<spdlog::logger> mAlertsLogger;
        libtorrent::settings_pack mSettingsPack;
        std::shared_ptr<libtorrent::session> mSession;
        std::vector<std::shared_ptr<Torrent>> mTorrents;
        std::shared_ptr<ServiceSettings> mSettings;
        mutable std::mutex mTorrentsMutex;
        mutable std::mutex mServiceMutex;
        mutable std::mutex mCvMutex;
        mutable std::condition_variable mCv;
        std::vector<std::thread> mThreads;
        std::atomic<bool> mIsRunning;
        std::int64_t mDownloadRate;
        std::int64_t mUploadRate;
        double mProgress;
        bool mRateLimited;
    };

}}

#endif //TORREST_SERVICE_H
