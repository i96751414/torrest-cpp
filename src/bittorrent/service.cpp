#include "service.h"

#include <thread>

#include "boost/filesystem.hpp"
#include "spdlog/sinks/stdout_sinks.h"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/write_resume_data.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/create_torrent.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/read_resume_data.hpp"

#include "utils/conversion.h"
#include "utils/filesystem.h"
#include "exceptions.h"

#define EXT_PARTS ".parts"
#define EXT_TORRENT ".torrent"
#define EXT_MAGNET ".magnet"
#define EXT_FASTRESUME ".fastresume"
#define MAX_FILES_PER_TORRENT 1000
#define MAX_SINGLE_CORE_CONNECTIONS 50
#define DEFAULT_DHT_BOOTSTRAP_NODES "router.utorrent.com:6881" \
                                    ",router.bittorrent.com:6881" \
                                    ",dht.transmissionbt.com:6881" \
                                    ",dht.aelitis.com:6881" \
                                    ",router.silotis.us:6881" \
                                    ",dht.libtorrent.org:25401"

#if TORRENT_ABI_VERSION <= 2
#define INFO_HASH_PARAM info_hash
#define INFO_HASH_T libtorrent::sha1_hash
#else
#define INFO_HASH_PARAM info_hashes
#define INFO_HASH_T libtorrent::info_hash_t
#endif

namespace {

    std::string get_info_hash(INFO_HASH_T const &pInfoHash) {
        std::stringstream ss;
#if TORRENT_ABI_VERSION <= 2
        ss << pInfoHash;
#else
        if (pInfoHash.has_v2()) {
            ss << pInfoHash.v2;
        } else {
            ss << pInfoHash.v1;
        }
#endif
        return ss.str();
    }

    bool seed_time_reached(int pSeedTimeLimit, const std::chrono::seconds &pSeedingTime) {
        return pSeedTimeLimit > 0 && pSeedingTime >= std::chrono::seconds(pSeedTimeLimit);
    }

    bool seed_time_ratio_reached(int pSeedTimeRatioLimit,
                                 const std::chrono::seconds &pDownloadTime,
                                 const std::chrono::seconds &pSeedingTime) {
        return pSeedTimeRatioLimit > 0
               && pDownloadTime > std::chrono::seconds::zero()
               && pSeedingTime * 100 / pDownloadTime >= pSeedTimeRatioLimit;
    }

    bool share_ratio_reached(int pShareRatioLimit, std::int64_t pAllTimeDownload, std::int64_t pAllTimeUpload) {
        return pShareRatioLimit > 0
               && pAllTimeDownload > 0
               && pAllTimeUpload * 100 / pAllTimeDownload >= pShareRatioLimit;
    }

}

namespace torrest { namespace bittorrent {

    struct Magnet {
        std::string magnet;
        bool download{};
    };

    std::ostream &operator<<(std::ostream &os, const Magnet &pMagnet) {
        auto size = pMagnet.magnet.size();
        os.write(reinterpret_cast<const char *>(&size), sizeof(size));
        os.write(pMagnet.magnet.data(), int(size));
        os.write(reinterpret_cast<const char *>(&pMagnet.download), sizeof(pMagnet.download));
        return os;
    }

    std::istream &operator>>(std::istream &is, Magnet &pMagnet) {
        std::string::size_type size;
        is.read(reinterpret_cast<char *>(&size), sizeof(size));
        pMagnet.magnet.resize(size);
        is.read(&pMagnet.magnet[0], int(size));
        is.read(reinterpret_cast<char *>(&pMagnet.download), sizeof(pMagnet.download));
        return is;
    }

    Service::Service(const settings::Settings &pSettings)
            : mLogger(spdlog::stdout_logger_mt("bittorrent")),
              mAlertsLogger(spdlog::stdout_logger_mt("alerts")),
              mIsRunning(true),
              mDownloadRate(0),
              mUploadRate(0),
              mProgress(0),
              mRateLimited(true),
              mSettings(std::make_shared<ServiceSettings>()) {

        configure(pSettings);
        mSession = std::make_shared<libtorrent::session>(mSettingsPack
#if TORRENT_ABI_VERSION <= 2
            , libtorrent::session::add_default_plugins
#endif
        );

        load_torrent_files();

        mThreads.emplace_back(&Service::check_save_resume_data_handler, this);
        mThreads.emplace_back(&Service::consume_alerts_handler, this);
        mThreads.emplace_back(&Service::piece_cleanup_handler, this);
        mThreads.emplace_back(&Service::progress_handler, this);
    }

    Service::~Service() {
        mIsRunning = false;
        mCv.notify_all();
        for (auto &thread : mThreads) {
            thread.join();
        }
    }

    void Service::check_save_resume_data_handler() const {
        mLogger->debug("operation=check_save_resume_data_handler, message='Initializing handler'");

        while (!wait_for_abort(mSettings->get_session_save())) {
            if (!mTorrents.empty()) {
                std::lock_guard<std::mutex> lock(mTorrentsMutex);
                for (auto &torrent : mTorrents) {
                    torrent->check_save_resume_data();
                }
            }
        }

        mLogger->debug("operation=check_save_resume_data_handler, message='Terminating handler'");
    }

    void Service::consume_alerts_handler() const {
        mLogger->debug("operation=consume_alerts_handler, message='Initializing handler'");
        libtorrent::seconds alertWaitTime{1};

        while (mIsRunning.load()) {
            if (mSession->wait_for_alert(alertWaitTime) == nullptr) {
                continue;
            }

            std::vector<libtorrent::alert *> alerts;
            mSession->pop_alerts(&alerts);

            for (auto alert : alerts) {
                auto alertMessage = alert->message();
                auto alertCategory = alert->category();

                switch (alert->type()) {
                    case libtorrent::save_resume_data_alert::alert_type:
                        handle_save_resume_data(dynamic_cast<const libtorrent::save_resume_data_alert *>(alert));
                        break;
                    case libtorrent::external_ip_alert::alert_type:
                        alertMessage = std::regex_replace(alertMessage, mIpRegex, ".XX");
                        break;
                    case libtorrent::metadata_received_alert::alert_type:
                        handle_metadata_received(dynamic_cast<const libtorrent::metadata_received_alert *>(alert));
                        break;
                    case libtorrent::state_changed_alert::alert_type:
                        handle_state_changed(dynamic_cast<const libtorrent::state_changed_alert *>(alert));
                        break;
                    case libtorrent::read_piece_alert::alert_type:
                        handle_read_piece_alert(dynamic_cast<const libtorrent::read_piece_alert *>(alert));
                        break;
                }

                spdlog::level::level_enum level;
                if (alertCategory & libtorrent::alert::error_notification) {
                    level = spdlog::level::err;
                } else if (alertCategory & libtorrent::alert::connect_notification) {
                    level = spdlog::level::debug;
                } else if (alertCategory & libtorrent::alert::performance_warning) {
                    level = spdlog::level::warn;
                } else {
                    level = spdlog::level::info;
                }

                mAlertsLogger->log(
                        level, "operation=consume_alerts_handler, type={}, what='{}', message='{}'",
                        alert->type(), alert->what(), alertMessage);
            }
        }

        mLogger->debug("operation=consume_alerts_handler, message='Terminating handler'");
    }

    void Service::handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert) const {
        auto infoHash = get_info_hash(pAlert->handle.INFO_HASH_PARAM());
        mLogger->debug("operation=handle_save_resume_data, message='Saving resume data', infoHash={}", infoHash);

        auto buffer = libtorrent::write_resume_data_buf(pAlert->params);
        std::ofstream of(get_fast_resume_file(infoHash), std::ios::binary);
        of.unsetf(std::ios::skipws);
        of.write(buffer.data(), int(buffer.size()));
    }

    void Service::handle_metadata_received(const libtorrent::metadata_received_alert *pAlert) const {
        auto torrentFile = pAlert->handle.torrent_file();
        auto infoHash = get_info_hash(torrentFile->INFO_HASH_PARAM());

        try {
            get_torrent(infoHash)->handle_metadata_received();
        } catch (const std::exception &e) {
            mLogger->error(
                    "operation=handle_metadata_received, message='Failed handling metadata', infoHash={}, what='{}'",
                    infoHash, e.what());
        }

        mLogger->debug("operation=handle_metadata_received, message='Saving torrent file', infoHash={}", infoHash);
        std::vector<char> buffer;
        libtorrent::create_torrent t(*torrentFile);
        libtorrent::bencode(std::back_inserter(buffer), t.generate());
        std::ofstream of(get_torrent_file(infoHash), std::ios::binary);
        of.unsetf(std::ios::skipws);
        of.write(buffer.data(), int(buffer.size()));

        mLogger->debug("operation=handle_metadata_received, message='Deleting magnet file', infoHash={}", infoHash);
        delete_magnet_file(infoHash);
    }

    void Service::handle_state_changed(const libtorrent::state_changed_alert *pAlert) const {
        if (mSettings->get_check_available_space() && pAlert->state == libtorrent::torrent_status::downloading) {
            auto infoHash = get_info_hash(pAlert->handle.INFO_HASH_PARAM());
            try {
                get_torrent(infoHash)->check_available_space(mSettings->get_download_path());
            } catch (const std::exception &e) {
                mLogger->error("operation=handle_state_changed, message='Failed handling state change', what='{}'",
                               e.what());
            }
        }
    }

    void Service::handle_read_piece_alert(const libtorrent::read_piece_alert *pAlert) const {
        auto infoHash = get_info_hash(pAlert->handle.INFO_HASH_PARAM());
        if (pAlert->error.failed()) {
            mLogger->error(
                    "operation=handle_read_piece_alert, message='Failed reading piece', infoHash={}, piece={}, error={}",
                    infoHash, to_string(pAlert->piece), pAlert->error.message());
        } else {
            try {
                get_torrent(infoHash)->store_piece(pAlert->piece, pAlert->size, pAlert->buffer);
            } catch (const std::exception &e) {
                mLogger->error("operation=handle_read_piece_alert, message='Failed handling read piece', what='{}'",
                               e.what());
            }
        }
    }

    void Service::piece_cleanup_handler() const {
        mLogger->debug("operation=piece_cleanup_handler, message='Initializing handler'");

        while (!wait_for_abort(2)) {
            if (!mTorrents.empty()) {
                std::lock_guard<std::mutex> lock(mTorrentsMutex);
                for (auto &torrent : mTorrents) {
                    torrent->cleanup_pieces(std::chrono::seconds(5));
                }
            }
        }

        mLogger->debug("operation=piece_cleanup_handler, message='Terminating handler'");
    }

    void Service::progress_handler() {
        mLogger->debug("operation=progress_handler, message='Initializing handler'");

        while (!wait_for_abort(1)) {
            if (!mSession->is_paused()) {
                update_progress();
            }
        }

        mLogger->debug("operation=progress_handler, message='Terminating handler'");
    }

    void Service::update_progress() {
        std::int64_t total_download_rate = 0;
        std::int64_t total_upload_rate = 0;
        std::int64_t total_wanted_done = 0;
        std::int64_t total_wanted = 0;
        bool has_files_buffering = false;

        std::unique_lock<std::mutex> lock(mTorrentsMutex);
        for (auto &torrent : mTorrents) {
            if (torrent->mPaused.load() || !torrent->mHasMetadata.load() || !torrent->mHandle.is_valid()) {
                continue;
            }

            // Check torrent buffering state
            if (torrent->verify_buffering_state()) {
                has_files_buffering = true;
            }

            auto status = torrent->mHandle.status();
            total_download_rate += status.download_rate;
            total_upload_rate += status.upload_rate;

            if (status.progress < 1) {
                total_wanted += status.total_wanted;
                total_wanted_done += status.total_wanted_done;
            } else {
                auto seeding_time = (status.progress == 1 && status.seeding_duration == std::chrono::seconds::zero())
                                    ? status.finished_duration : status.seeding_duration;
                auto download_time = status.active_duration - seeding_time;

                if (seed_time_reached(mSettings->get_seed_time_limit(), seeding_time)) {
                    mLogger->info("operation=update_progress, message='Seeding time limit reached', infoHash={}",
                                  torrent->mInfoHash);
                    torrent->pause();
                } else if (seed_time_ratio_reached(mSettings->get_seed_time_ratio_limit(), download_time,
                                                   seeding_time)) {
                    mLogger->info("operation=update_progress, message='Seeding time ratio reached', infoHash={}",
                                  torrent->mInfoHash);
                    torrent->pause();
                } else if (share_ratio_reached(mSettings->get_share_ratio_limit(), status.all_time_download,
                                               status.all_time_upload)) {
                    mLogger->info("operation=update_progress, message='Share ratio reached', infoHash={}",
                                  torrent->mInfoHash);
                    torrent->pause();
                }
            }
        }

        lock.unlock();
        std::lock_guard<std::mutex> sLock(mServiceMutex);
        set_buffering_rate_limits(!has_files_buffering);

        mDownloadRate = total_download_rate;
        mUploadRate = total_upload_rate;
        mProgress = total_wanted > 0
                    ? 100 * static_cast<double>(total_wanted_done) / static_cast<double>(total_wanted) : 100;
    }

    void Service::reconfigure(const settings::Settings &pSettings, bool pReset) {
        mLogger->debug("operation=reconfigure, message='Reconfiguring service', reset={}", pReset);
        std::lock_guard<std::mutex> lock(mServiceMutex);
        configure(pSettings);
        mSession->apply_settings(mSettingsPack);

        if (pReset) {
            mLogger->debug("operation=reconfigure, message='Resetting torrents'");
            std::lock_guard<std::mutex> tLock(mTorrentsMutex);
            remove_torrents();
            load_torrent_files();
        }
    }

    void Service::configure(const settings::Settings &pSettings) {
        boost::filesystem::create_directory(pSettings.download_path);
        boost::filesystem::create_directory(pSettings.torrents_path);

        mLogger->set_level(pSettings.service_log_level);
        mAlertsLogger->set_level(pSettings.alerts_log_level);
        mLogger->info("operation=configure, message='Applying session settings'");
        mSettings->update(pSettings);

        auto userAgent = get_user_agent(pSettings.user_agent);
        mLogger->debug("operation=configure, userAgent='{}'", userAgent);
        mSettingsPack.set_str(libtorrent::settings_pack::user_agent, userAgent);

        // Default settings
        mSettingsPack.set_int(libtorrent::settings_pack::request_timeout, 2);
        mSettingsPack.set_int(libtorrent::settings_pack::peer_connect_timeout, 2);
        mSettingsPack.set_bool(libtorrent::settings_pack::strict_end_game_mode, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::announce_to_all_trackers, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::announce_to_all_tiers, true);
        mSettingsPack.set_int(libtorrent::settings_pack::connection_speed, 500);
        mSettingsPack.set_int(libtorrent::settings_pack::choking_algorithm,
                              libtorrent::settings_pack::fixed_slots_choker);
        mSettingsPack.set_int(libtorrent::settings_pack::share_ratio_limit, 0);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_ratio_limit, 0);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_limit, 0);
        mSettingsPack.set_int(libtorrent::settings_pack::peer_tos, tos_min_delay);
        mSettingsPack.set_int(libtorrent::settings_pack::torrent_connect_boost, 0);
        mSettingsPack.set_bool(libtorrent::settings_pack::rate_limit_ip_overhead, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::no_atime_storage, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::deprecated_announce_double_nat, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::prioritize_partial_pieces, false);
        mSettingsPack.set_bool(libtorrent::settings_pack::use_parole_mode, true);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_choking_algorithm,
                              libtorrent::settings_pack::fastest_upload);
        mSettingsPack.set_bool(libtorrent::settings_pack::upnp_ignore_nonrouters, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::deprecated_lazy_bitfield, true);
        mSettingsPack.set_int(libtorrent::settings_pack::stop_tracker_timeout, 1);
        mSettingsPack.set_int(libtorrent::settings_pack::auto_scrape_interval, 1200);
        mSettingsPack.set_int(libtorrent::settings_pack::auto_scrape_min_interval, 900);
        mSettingsPack.set_bool(libtorrent::settings_pack::deprecated_ignore_limits_on_local_network, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::deprecated_rate_limit_utp, true);
        mSettingsPack.set_int(libtorrent::settings_pack::mixed_mode_algorithm, libtorrent::settings_pack::prefer_tcp);

        // For Android external storage / OS-mounted NAS setups
        if (pSettings.tuned_storage) {
            mSettingsPack.set_int(libtorrent::settings_pack::max_queued_disk_bytes, 10 * 1024 * 1024);
#if TORRENT_ABI_VERSION <= 2
            mSettingsPack.set_bool(libtorrent::settings_pack::use_read_cache, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::coalesce_reads, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::coalesce_writes, true);
            mSettingsPack.set_int(libtorrent::settings_pack::cache_size, -1);
#endif
        }

        mSettingsPack.set_int(libtorrent::settings_pack::connections_limit,
                              pSettings.connections_limit > 0 ? pSettings.connections_limit : 200);
#ifdef __arm__
        if (std::thread::hardware_concurrency() == 1) {
            mLogger->debug(
                    "operation=configure, message='Setting max single core connections limit', connectionsLimit={}",
                    MAX_SINGLE_CORE_CONNECTIONS);
            mSettingsPack.set_int(libtorrent::settings_pack::connections_limit, MAX_SINGLE_CORE_CONNECTIONS);
        }
#endif

        if (!pSettings.limit_after_buffering || mRateLimited) {
            mSettingsPack.set_int(libtorrent::settings_pack::download_rate_limit, pSettings.max_download_rate);
            mSettingsPack.set_int(libtorrent::settings_pack::upload_rate_limit, pSettings.max_upload_rate);
            mRateLimited = true;
        }

        mSettingsPack.set_int(libtorrent::settings_pack::share_ratio_limit,
                              pSettings.share_ratio_limit > 0 ? pSettings.share_ratio_limit : 200);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_ratio_limit,
                              pSettings.seed_time_ratio_limit > 0 ? pSettings.seed_time_ratio_limit : 700);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_limit,
                              pSettings.seed_time_limit > 0 ? pSettings.seed_time_limit : 24 * 60 * 60);

        mSettingsPack.set_int(libtorrent::settings_pack::active_downloads, pSettings.active_downloads_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_seeds, pSettings.active_seeds_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_checking, pSettings.active_checking_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_dht_limit, pSettings.active_dht_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_tracker_limit, pSettings.active_tracker_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_lsd_limit, pSettings.active_lsd_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_limit, pSettings.active_limit);

        libtorrent::settings_pack::enc_policy encPolicy;
        libtorrent::settings_pack::enc_level encLevel;
        bool preferRc4;

        switch (pSettings.encryption_policy) {
            case settings::ep_disabled:
                encPolicy = libtorrent::settings_pack::pe_disabled;
                encLevel = libtorrent::settings_pack::pe_both;
                preferRc4 = false;
                break;
            case settings::ep_forced:
                encPolicy = libtorrent::settings_pack::pe_forced;
                encLevel = libtorrent::settings_pack::pe_rc4;
                preferRc4 = true;
                break;
            default:
                encPolicy = libtorrent::settings_pack::pe_enabled;
                encLevel = libtorrent::settings_pack::pe_both;
                preferRc4 = false;
        }

        mLogger->debug(
                "operation=configure, message='Applying encryption settings', encPolicy={}, encLevel={}, preferRc4={}",
                encPolicy, encLevel, preferRc4);
        mSettingsPack.set_int(libtorrent::settings_pack::out_enc_policy, encPolicy);
        mSettingsPack.set_int(libtorrent::settings_pack::in_enc_policy, encPolicy);
        mSettingsPack.set_int(libtorrent::settings_pack::allowed_enc_level, encLevel);
        mSettingsPack.set_bool(libtorrent::settings_pack::prefer_rc4, preferRc4);

        if (pSettings.proxy && pSettings.proxy->type != settings::pt_none) {
            libtorrent::settings_pack::proxy_type_t proxyType;

            switch (pSettings.proxy->type) {
                case settings::pt_socks4:
                    proxyType = libtorrent::settings_pack::socks4;
                    break;
                case settings::pt_socks5:
                    proxyType = libtorrent::settings_pack::socks5;
                    break;
                case settings::pt_socks5_password:
                    proxyType = libtorrent::settings_pack::socks5_pw;
                    break;
                case settings::pt_http:
                    proxyType = libtorrent::settings_pack::http;
                    break;
                case settings::pt_http_password:
                    proxyType = libtorrent::settings_pack::http_pw;
                    break;
                case settings::pt_i2psam:
                    proxyType = libtorrent::settings_pack::i2p_proxy;
                    mSettingsPack.set_int(libtorrent::settings_pack::i2p_port, pSettings.proxy->port);
                    mSettingsPack.set_str(libtorrent::settings_pack::i2p_hostname, pSettings.proxy->hostname);
                    mSettingsPack.set_bool(libtorrent::settings_pack::allow_i2p_mixed, true);
                    break;
                default:
                    mLogger->warn("operation=configure, message='Unknown proxy type', proxyType={}",
                                  pSettings.proxy->type);
                    proxyType = libtorrent::settings_pack::none;
            }

            mLogger->debug("operation=configure, message='Applying proxy settings', proxyType={}", proxyType);
            mSettingsPack.set_int(libtorrent::settings_pack::proxy_type, proxyType);
            mSettingsPack.set_int(libtorrent::settings_pack::proxy_port, pSettings.proxy->port);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_hostname, pSettings.proxy->hostname);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_username, pSettings.proxy->username);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_password, pSettings.proxy->password);
            mSettingsPack.set_bool(libtorrent::settings_pack::proxy_tracker_connections, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::proxy_peer_connections, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::proxy_hostnames, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::deprecated_force_proxy, true);
        }

        mSettingsPack.set_int(libtorrent::settings_pack::alert_mask,
                              libtorrent::alert::status_notification
                              | libtorrent::alert::storage_notification
                              | libtorrent::alert::error_notification);

        std::vector<std::string> listenInterfaces;
        auto listenPort = ":" + std::to_string(pSettings.listen_port);
        auto configListenInterfaces = std::regex_replace(pSettings.listen_interfaces, mWhiteSpaceRegex, "");

        if (configListenInterfaces.empty()) {
            listenInterfaces = {"0.0.0.0" + listenPort, "[::]" + listenPort};
        } else {
            std::stringstream ss(configListenInterfaces);
            while (ss.good()) {
                std::string interface;
                std::getline(ss, interface, ',');

                if (!std::regex_match(interface, mPortRegex)) {
                    interface += listenPort;
                }

                listenInterfaces.push_back(interface);
            }
        }

        auto listenInterfacesStr = utils::join_string_vector(listenInterfaces, ",");
        mLogger->debug("operation=configure, message='Setting listen interfaces', listenInterfaces='{}'",
                       listenInterfacesStr);
        mSettingsPack.set_str(libtorrent::settings_pack::listen_interfaces, listenInterfacesStr);

        auto outgoingInterfaces = std::regex_replace(pSettings.outgoing_interfaces, mWhiteSpaceRegex, "");
        if (!outgoingInterfaces.empty()) {
            mSettingsPack.set_str(libtorrent::settings_pack::outgoing_interfaces, outgoingInterfaces);
        }

        mSettingsPack.set_str(libtorrent::settings_pack::dht_bootstrap_nodes, DEFAULT_DHT_BOOTSTRAP_NODES);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_dht, !pSettings.disable_dht);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_upnp, !pSettings.disable_upnp);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_natpmp, !pSettings.disable_natpmp);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_lsd, !pSettings.disable_lsd);
    }

    void Service::set_buffering_rate_limits(bool pEnable) {
        if (mSettings->get_limit_after_buffering() && mRateLimited != pEnable) {
            mLogger->debug("operation=set_buffering_rate_limits, enable={}", pEnable);
            mSettingsPack.set_int(libtorrent::settings_pack::download_rate_limit,
                                  pEnable ? mSettings->get_max_download_rate() : 0);
            mSettingsPack.set_int(libtorrent::settings_pack::upload_rate_limit,
                                  pEnable ? mSettings->get_max_upload_rate() : 0);
            mSession->apply_settings(mSettingsPack);
            mRateLimited = pEnable;
        }
    }

    void Service::remove_torrents() {
        mLogger->debug("operation=remove_torrents, message='Removing all torrents'");
        for (auto it = mTorrents.begin(); it != mTorrents.end(); it = mTorrents.erase(it)) {
            (*it)->mClosed = true;
            mSession->remove_torrent((*it)->mHandle);
        }
    }

    void Service::add_torrent_with_params(libtorrent::add_torrent_params &pTorrentParams,
                                          const std::string &pInfoHash,
                                          bool pIsResumeData,
                                          bool pDownload) {
        mLogger->debug("operation=add_torrent_with_params, message='Adding torrent', infoHash={}", pInfoHash);

        if (has_torrent(pInfoHash)) {
            throw DuplicateTorrentException("Torrent was previously added", pInfoHash);
        }

        if (!pIsResumeData) {
            mLogger->debug("operation=add_torrent_with_params, message='Setting params', infoHash={}", pInfoHash);
            pTorrentParams.save_path = mSettings->get_download_path();
            pTorrentParams.flags |= libtorrent::torrent_flags::sequential_download;
        }

        if (!pDownload) {
            mLogger->debug("operation=add_torrent_with_params, message='Disabling download', infoHash={}", pInfoHash);
            for (int i = MAX_FILES_PER_TORRENT; i > 0; i--) {
                pTorrentParams.file_priorities.push_back(libtorrent::download_priority_t(0));
            }
        }

        libtorrent::error_code errorCode;
        auto handle = mSession->add_torrent(pTorrentParams, errorCode);
        if (errorCode.failed() || !handle.is_valid()) {
            mLogger->error("operation=add_torrent_with_params, message='{}', infoHash={}",
                           errorCode.message(), pInfoHash);
            throw LoadTorrentException(errorCode.message());
        }

        auto torrent = std::make_shared<Torrent>(mSettings, handle, pInfoHash, mLogger);
        if (pTorrentParams.ti != nullptr && pTorrentParams.ti->is_valid()) {
            torrent->handle_metadata_received();
        }
        mTorrents.emplace_back(torrent);
    }

    std::string Service::add_magnet(const std::string &pMagnet, bool pDownload, bool pSaveMagnet) {
        mLogger->debug("operation=add_magnet, message='Adding magnet', magnet='{}', download={}, saveMagnet={}",
                       pMagnet, pDownload, pSaveMagnet);
        libtorrent::add_torrent_params torrentParams;
        libtorrent::error_code errorCode;
        libtorrent::parse_magnet_uri(pMagnet, torrentParams, errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_magnet, message='Failed parsing magnet: {}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.INFO_HASH_PARAM);
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);

        if (pSaveMagnet) {
            std::ofstream of(get_magnet_file(infoHash), std::ios::binary);
            of << Magnet{pMagnet, pDownload};
        }

        return infoHash;
    }

    std::string Service::add_magnet(const std::string &pMagnet, bool pDownload) {
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        return add_magnet(pMagnet, pDownload, true);
    }

    std::string Service::add_torrent_data(const char *pData, int pSize, bool pDownload) {
        mLogger->debug("operation=add_torrent_data, message='Adding torrent data', download={}", pDownload);
        libtorrent::error_code errorCode;
        libtorrent::add_torrent_params torrentParams;
        torrentParams.ti = std::make_shared<libtorrent::torrent_info>(pData, pSize, errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_torrent_data, message='Failed adding torrent data: {}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.ti->INFO_HASH_PARAM());
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);
        std::ofstream of(get_torrent_file(infoHash), std::ios::binary);
        of.write(pData, pSize);

        return infoHash;
    }

    std::string Service::add_torrent_file(const std::string &pFile, bool pDownload, bool pSaveFile) {
        mLogger->debug("operation=add_torrent_file, message='Adding torrent file', download={}, saveFile={}",
                       pDownload, pSaveFile);
        libtorrent::error_code errorCode;
        libtorrent::add_torrent_params torrentParams;
        torrentParams.ti = std::make_shared<libtorrent::torrent_info>(pFile, errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_torrent_file, message='Failed adding torrent: {}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.ti->INFO_HASH_PARAM());
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);

        if (pSaveFile) {
            auto destPath = get_torrent_file(infoHash);
            if (!boost::filesystem::equivalent(pFile, destPath)) {
                boost::filesystem::copy_file(pFile, destPath);
            }
        }

        return infoHash;
    }

    std::string Service::add_torrent_file(const std::string &pFile, bool pDownload) {
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        return add_torrent_file(pFile, pDownload, true);
    }

    void Service::add_torrent_with_resume_data(const std::string &pFile) {
        mLogger->debug("operation=add_torrent_with_resume_data");
        std::ifstream ifs(pFile, std::ios::binary);
        ifs.unsetf(std::ios::skipws);
        std::vector<char> resumeData{std::istream_iterator<char>(ifs), std::istream_iterator<char>()};

        libtorrent::error_code errorCode;
        auto torrentParams = libtorrent::read_resume_data(resumeData, errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_torrent_with_resume_data, message='{}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.INFO_HASH_PARAM);
        add_torrent_with_params(torrentParams, infoHash, true, true);
    }

    void Service::load_torrent_files() {
        mLogger->debug("operation=load_torrent_files, message='Loading torrent files'");
        std::vector<std::string> fastResumeFiles;
        std::vector<std::string> torrentFiles;
        std::vector<std::string> magnetFiles;

        for (auto &p : boost::filesystem::directory_iterator(mSettings->get_torrents_path())) {
            if (boost::filesystem::is_regular_file(p.path())) {
                auto ext = p.path().extension();
                if (ext == EXT_FASTRESUME) {
                    fastResumeFiles.emplace_back(p.path().string());
                } else if (ext == EXT_TORRENT) {
                    torrentFiles.emplace_back(p.path().string());
                } else if (ext == EXT_MAGNET) {
                    magnetFiles.emplace_back(p.path().string());
                }
            }
        }

        for (auto &f : fastResumeFiles) {
            try {
                add_torrent_with_resume_data(f);
            } catch (const BittorrentException &e) {
                mLogger->error("operation=load_torrent_files, message='Failed adding torrent with resume data'"
                               ", what='{}', file='{}'", e.what(), f);
                boost::filesystem::remove(f);
            }
        }

        for (auto &f : torrentFiles) {
            try {
                add_torrent_file(f, false, false);
            } catch (const LoadTorrentException &e) {
                mLogger->error("operation=load_torrent_files, message='Failed loading torrent', what='{}', file='{}'",
                               e.what(), f);
                boost::filesystem::remove(f);
            } catch (const DuplicateTorrentException &e) {
                mLogger->debug("operation=load_torrent_files, message='{}', infoHash={}, file='{}'",
                               e.what(), e.get_info_hash(), f);
            } catch (const BittorrentException &e) {
                mLogger->error("operation=load_torrent_files, message='Failed adding torrent', what='{}', file='{}'",
                               e.what(), f);
            }
        }

        for (auto &f : magnetFiles) {
            Magnet m;
            try {
                std::ifstream ifs(f, std::ios::binary);
                ifs >> m;
            } catch (const std::exception &e) {
                mLogger->error("operation=load_torrent_files, message='Failed parsing magnet file', what='{}'",
                               e.what());
                continue;
            }

            try {
                add_magnet(m.magnet, m.download, false);
            } catch (const DuplicateTorrentException &e) {
                mLogger->debug("operation=load_torrent_files, message='Deleting duplicate magnet', infoHash={}",
                               e.get_info_hash());
                delete_magnet_file(e.get_info_hash());
            } catch (const BittorrentException &e) {
                mLogger->error("operation=load_torrent_files, message='Failed adding magnet', what='{}', file='{}'",
                               e.what(), f);
                boost::filesystem::remove(f);
            }
        }

        for (auto &p : boost::filesystem::directory_iterator(mSettings->get_download_path())) {
            if (p.path().extension() == EXT_PARTS && boost::filesystem::is_regular_file(p.path())) {
                auto infoHash = utils::ltrim_copy(p.path().stem().string(), ".");
                if (!has_torrent(infoHash)) {
                    mLogger->debug("operation=load_torrent_files, message='Cleaning stale parts', infoHash={}",
                                   infoHash);
                    boost::filesystem::remove(p.path());
                }
            }
        }
    }

    std::vector<std::shared_ptr<Torrent>>::const_iterator Service::find_torrent(const std::string &pInfoHash,
                                                                                bool pMustFind) const {
        mLogger->trace("operation=find_torrent, infoHash={}", pInfoHash);
        auto torrent = std::find_if(
                mTorrents.begin(), mTorrents.end(),
                [&pInfoHash](const std::shared_ptr<Torrent> &t) { return t->get_info_hash() == pInfoHash; });

        if (pMustFind && torrent == mTorrents.end()) {
            mLogger->error("operation=get_torrent, message='Unable to find torrent', infoHash={}", pInfoHash);
            throw InvalidInfoHashException("No such info hash");
        }

        return torrent;
    }

    bool Service::has_torrent(const std::string &pInfoHash) const {
        return find_torrent(pInfoHash, false) != mTorrents.end();
    }

    std::shared_ptr<Torrent> Service::get_torrent(const std::string &pInfoHash) const {
        mLogger->trace("operation=get_torrent, infoHash={}", pInfoHash);
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        return *find_torrent(pInfoHash);
    }

    std::vector<std::shared_ptr<Torrent>> Service::get_torrents() const {
        mLogger->trace("operation=get_torrents");
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        return mTorrents;
    }

    void Service::remove_torrent(const std::string &pInfoHash, bool pRemoveFiles) {
        mLogger->debug("operation=remove_torrent, infoHash={}, removeFiles={}", pInfoHash, pRemoveFiles);
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        auto it = find_torrent(pInfoHash);

        delete_parts_file(pInfoHash);
        delete_fast_resume_file(pInfoHash);
        delete_torrent_file(pInfoHash);
        delete_magnet_file(pInfoHash);

        (*it)->mClosed = true;
        mSession->remove_torrent(
                (*it)->mHandle,
                pRemoveFiles ? libtorrent::session_handle::delete_files : libtorrent::remove_flags_t(0));

        mTorrents.erase(it);
    }

    ServiceStatus Service::get_status() const {
        mLogger->trace("operation=get_status");
        std::lock_guard<std::mutex> lock(mServiceMutex);
        return ServiceStatus{
                .progress=mProgress,
                .download_rate=mDownloadRate,
                .upload_rate=mUploadRate,
                .num_torrents=static_cast<int>(mTorrents.size()),
                .paused=mSession->is_paused(),
        };
    }

    void Service::pause() {
        mLogger->debug("operation=pause, message='Pausing service'");
        mSession->pause();
    }

    void Service::resume() {
        mLogger->debug("operation=resume, message='Resuming service'");
        mSession->resume();
    }

    inline std::string Service::get_parts_file(const std::string &pInfoHash) const {
        return utils::join_path(mSettings->get_download_path(), "." + pInfoHash + EXT_PARTS).string();
    }

    inline std::string Service::get_fast_resume_file(const std::string &pInfoHash) const {
        return utils::join_path(mSettings->get_torrents_path(), pInfoHash + EXT_FASTRESUME).string();
    }

    inline std::string Service::get_torrent_file(const std::string &pInfoHash) const {
        return utils::join_path(mSettings->get_torrents_path(), pInfoHash + EXT_TORRENT).string();
    }

    inline std::string Service::get_magnet_file(const std::string &pInfoHash) const {
        return utils::join_path(mSettings->get_torrents_path(), pInfoHash + EXT_MAGNET).string();
    }

    inline void Service::delete_parts_file(const std::string &pInfoHash) const {
        boost::filesystem::remove(get_parts_file(pInfoHash));
    }

    inline void Service::delete_fast_resume_file(const std::string &pInfoHash) const {
        boost::filesystem::remove(get_fast_resume_file(pInfoHash));
    }

    inline void Service::delete_torrent_file(const std::string &pInfoHash) const {
        boost::filesystem::remove(get_torrent_file(pInfoHash));
    }

    inline void Service::delete_magnet_file(const std::string &pInfoHash) const {
        boost::filesystem::remove(get_magnet_file(pInfoHash));
    }

    bool Service::wait_for_abort(const int &pSeconds) const {
        return wait_for_abort(std::chrono::seconds(pSeconds));
    }

    bool Service::wait_for_abort(const std::chrono::seconds &pSeconds) const {
        auto until = std::chrono::steady_clock::now() + pSeconds;
        std::unique_lock<std::mutex> lock(mCvMutex);
        return mCv.wait_until(lock, until, [this] { return !mIsRunning.load(); });
    }

}}
