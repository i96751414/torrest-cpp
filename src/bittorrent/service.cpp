#include "service.h"

#include <thread>
#include <experimental/filesystem>

#include "spdlog/sinks/stdout_sinks.h"
#include <libtorrent/alert_types.hpp>

#include "utils/conversion.h"
#include "utils/filesystem.h"

#define EXT_PARTS ".parts"
#define EXT_TORRENT ".torrent"
#define EXT_MAGNET ".magnet"
#define EXT_FASTRESUME ".fastresume"
#define MAX_SINGLE_CORE_CONNECTIONS 50
#define DEFAULT_DHT_BOOTSTRAP_NODES "router.utorrent.com:6881" \
                                    ",router.bittorrent.com:6881" \
                                    ",dht.transmissionbt.com:6881" \
                                    ",dht.aelitis.com:6881" \
                                    ",router.silotis.us:6881" \
                                    ",dht.libtorrent.org:25401"

namespace torrest {

    enum PeerTos {
        tos_normal = 0x00,
        tos_min_cost = 0x02,
        tos_max_reliability = 0x04,
        tos_max_throughput = 0x08,
        tos_min_delay = 0x10,
        tos_scavenger = 0x20
    };

    Service::Service(const Settings &pSettings)
            : mLogger(spdlog::stdout_logger_mt("bittorrent")),
              mAlertsLogger(spdlog::stdout_logger_mt("alerts")),
              mIsRunning(true) {

        configure(pSettings);
        mSession = std::make_shared<libtorrent::session>(mSettingsPack, libtorrent::session::add_default_plugins);

        mThreads.emplace_back(&Service::consume_alerts_handler, this);
    }

    Service::~Service() {
        mIsRunning = false;
        for (auto &thread : mThreads) {
            thread.join();
        }
    }

    void Service::consume_alerts_handler() {
        mLogger->debug("operation=consume_alerts_handler, message='Initializing alerts consumer'");
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

        mLogger->debug("operation=consume_alerts_handler, message='Terminating alerts consumer'");
    }

    void Service::handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert) {
        // TODO
    }

    void Service::handle_metadata_received(const libtorrent::metadata_received_alert *pAlert) {
        // TODO
    }

    void Service::handle_state_changed(const libtorrent::state_changed_alert *pAlert) {
        // TODO
    }

    void Service::reconfigure(const Settings &pSettings, bool pReset) {
        mLogger->debug("operation=reconfigure, message='Reconfiguring service', reset={}", pReset);
        std::lock_guard<std::mutex> lock(mMutex);

        configure(pSettings);
        mSession->apply_settings(mSettingsPack);

        if (pReset) {
            mLogger->debug("operation=reconfigure, message='Resetting torrents'");
            // TODO: Reset torrents
        }
    }

    void Service::configure(const Settings &pSettings) {
        std::experimental::filesystem::create_directory(pSettings.download_path);
        std::experimental::filesystem::create_directory(pSettings.torrents_path);
        mSettings = pSettings;

        mLogger->set_level(mSettings.service_log_level);
        mAlertsLogger->set_level(mSettings.alert_log_level);
        mLogger->info("operation=configure, message='Applying session settings'");

        auto userAgent = get_user_agent(mSettings.user_agent);
        mLogger->debug("operation=configure, userAgent='{}'", userAgent);
        mSettingsPack.set_str(libtorrent::settings_pack::user_agent, userAgent);

        // Default settings
        mSettingsPack.set_int(libtorrent::settings_pack::request_timeout, 2);
        mSettingsPack.set_int(libtorrent::settings_pack::peer_connect_timeout, 2);
        mSettingsPack.set_bool(libtorrent::settings_pack::strict_end_game_mode, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::announce_to_all_trackers, true);
        mSettingsPack.set_bool(libtorrent::settings_pack::announce_to_all_tiers, true);
        mSettingsPack.set_int(libtorrent::settings_pack::connection_speed, 500);
        mSettingsPack.set_int(libtorrent::settings_pack::download_rate_limit, 0);
        mSettingsPack.set_int(libtorrent::settings_pack::upload_rate_limit, 0);
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
        if (mSettings.tuned_storage) {
            mSettingsPack.set_bool(libtorrent::settings_pack::use_read_cache, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::coalesce_reads, true);
            mSettingsPack.set_bool(libtorrent::settings_pack::coalesce_writes, true);
            mSettingsPack.set_int(libtorrent::settings_pack::max_queued_disk_bytes, 10 * 1024 * 1024);
            mSettingsPack.set_int(libtorrent::settings_pack::cache_size, -1);
        }

        mSettingsPack.set_int(libtorrent::settings_pack::connections_limit,
                              mSettings.connections_limit > 0 ? mSettings.connections_limit : 200);
#ifdef __arm__
        if (std::thread::hardware_concurrency() == 1) {
            mLogger->debug(
                    "operation=configure, message='Setting max single core connections limit', connectionsLimit={}",
                    MAX_SINGLE_CORE_CONNECTIONS);
            mSettingsPack.set_int(libtorrent::settings_pack::connections_limit, MAX_SINGLE_CORE_CONNECTIONS);
        }
#endif

        if (!mSettings.limit_after_buffering) {
            mSettingsPack.set_int(libtorrent::settings_pack::download_rate_limit, mSettings.max_download_rate);
            mSettingsPack.set_int(libtorrent::settings_pack::upload_rate_limit, mSettings.max_upload_rate);
        }

        mSettingsPack.set_int(libtorrent::settings_pack::share_ratio_limit,
                              mSettings.share_ratio_limit > 0 ? mSettings.share_ratio_limit : 200);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_ratio_limit,
                              mSettings.seed_time_ratio_limit > 0 ? mSettings.seed_time_ratio_limit : 700);
        mSettingsPack.set_int(libtorrent::settings_pack::seed_time_limit,
                              mSettings.seed_time_limit > 0 ? mSettings.seed_time_limit : 24 * 60 * 60);

        mSettingsPack.set_int(libtorrent::settings_pack::active_downloads, mSettings.active_downloads_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_seeds, mSettings.active_seeds_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_checking, mSettings.active_checking_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_dht_limit, mSettings.active_dht_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_tracker_limit, mSettings.active_tracker_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_lsd_limit, mSettings.active_lsd_limit);
        mSettingsPack.set_int(libtorrent::settings_pack::active_limit, mSettings.active_limit);

        libtorrent::settings_pack::enc_policy encPolicy;
        libtorrent::settings_pack::enc_level encLevel;
        bool preferRc4;

        switch (mSettings.encryption_policy) {
            case ep_disabled:
                encPolicy = libtorrent::settings_pack::pe_disabled;
                encLevel = libtorrent::settings_pack::pe_both;
                preferRc4 = false;
                break;
            case ep_forced:
                encPolicy = libtorrent::settings_pack::pe_forced;
                encLevel = libtorrent::settings_pack::pe_rc4;
                preferRc4 = true;
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

        if (mSettings.proxy_type != pt_none) {
            libtorrent::settings_pack::proxy_type_t proxyType;

            switch (mSettings.proxy_type) {
                case pt_socks4:
                    proxyType = libtorrent::settings_pack::socks4;
                    break;
                case pt_socks5:
                    proxyType = libtorrent::settings_pack::socks5;
                    break;
                case pt_socks5_password:
                    proxyType = libtorrent::settings_pack::socks5_pw;
                    break;
                case pt_http:
                    proxyType = libtorrent::settings_pack::http;
                    break;
                case pt_http_password:
                    proxyType = libtorrent::settings_pack::http_pw;
                    break;
                case pt_i2psam:
                    proxyType = libtorrent::settings_pack::i2p_proxy;
                    mSettingsPack.set_int(libtorrent::settings_pack::i2p_port, mSettings.proxy_port);
                    mSettingsPack.set_str(libtorrent::settings_pack::i2p_hostname, mSettings.proxy_hostname);
                    mSettingsPack.set_bool(libtorrent::settings_pack::allow_i2p_mixed, true);
                    break;
                default:
                    mLogger->warn("operation=configure, message='Unknown proxy type', proxyType={}",
                                  mSettings.proxy_type);
                    proxyType = libtorrent::settings_pack::none;
            }

            mLogger->debug("operation=configure, message='Applying proxy settings', proxyType={}", proxyType);
            mSettingsPack.set_int(libtorrent::settings_pack::proxy_type, proxyType);
            mSettingsPack.set_int(libtorrent::settings_pack::proxy_port, mSettings.proxy_port);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_hostname, mSettings.proxy_hostname);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_username, mSettings.proxy_username);
            mSettingsPack.set_str(libtorrent::settings_pack::proxy_password, mSettings.proxy_password);
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
        auto listenPort = ":" + std::to_string(mSettings.listen_port);
        auto configListenInterfaces = std::regex_replace(mSettings.listen_interfaces, mWhiteSpaceRegex, "");

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

        auto listenInterfacesStr = join_string_vector(listenInterfaces, ",");
        mLogger->debug("operation=configure, message='Setting listen interfaces', listenInterfaces='{}'",
                       listenInterfacesStr);
        mSettingsPack.set_str(libtorrent::settings_pack::listen_interfaces, listenInterfacesStr);

        auto outgoingInterfaces = std::regex_replace(mSettings.outgoing_interfaces, mWhiteSpaceRegex, "");
        if (!outgoingInterfaces.empty()) {
            mSettingsPack.set_str(libtorrent::settings_pack::outgoing_interfaces, outgoingInterfaces);
        }

        mSettingsPack.set_str(libtorrent::settings_pack::dht_bootstrap_nodes, DEFAULT_DHT_BOOTSTRAP_NODES);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_dht, !mSettings.disable_dht);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_upnp, !mSettings.disable_upnp);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_natpmp, !mSettings.disable_natpmp);
        mSettingsPack.set_bool(libtorrent::settings_pack::enable_lsd, !mSettings.disable_lsd);
    }

    void Service::set_buffering_rate_limits(bool pEnable) {
        if (mSettings.limit_after_buffering) {
            mLogger->debug("operation=set_buffering_rate_limits, enable={}", pEnable);
            mSettingsPack.set_int(libtorrent::settings_pack::download_rate_limit,
                                  pEnable ? mSettings.max_download_rate : 0);
            mSettingsPack.set_int(libtorrent::settings_pack::upload_rate_limit,
                                  pEnable ? mSettings.max_upload_rate : 0);
            mSession->apply_settings(mSettingsPack);
        }
    }

    inline std::string Service::get_parts_file(const std::string &pInfoHash) const {
        return join_path(mSettings.download_path, "." + pInfoHash + EXT_PARTS);
    }

    inline std::string Service::get_fast_resume_file(const std::string &pInfoHash) const {
        return join_path(mSettings.torrents_path, pInfoHash + EXT_FASTRESUME);
    }

    inline std::string Service::get_torrent_file(const std::string &pInfoHash) const {
        return join_path(mSettings.torrents_path, pInfoHash + EXT_TORRENT);
    }

    inline std::string Service::get_magnet_file(const std::string &pInfoHash) const {
        return join_path(mSettings.torrents_path, pInfoHash + EXT_MAGNET);
    }

    inline void Service::delete_parts_file(const std::string &pInfoHash) const {
        std::experimental::filesystem::remove(get_parts_file(pInfoHash));
    }

    inline void Service::delete_fast_resume_file(const std::string &pInfoHash) const {
        std::experimental::filesystem::remove(get_fast_resume_file(pInfoHash));
    }

    inline void Service::delete_torrent_file(const std::string &pInfoHash) const {
        std::experimental::filesystem::remove(get_torrent_file(pInfoHash));
    }

    inline void Service::delete_magnet_file(const std::string &pInfoHash) const {
        std::experimental::filesystem::remove(get_magnet_file(pInfoHash));
    }
}