#include "service.h"

#include <thread>
#include <experimental/filesystem>

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

namespace {

    std::string get_info_hash(libtorrent::sha1_hash const &pSha1Hash) {
        std::stringstream ss;
        ss << pSha1Hash;
        return ss.str();
    }

}

namespace torrest {

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

    Service::Service(const Settings &pSettings)
            : mLogger(spdlog::stdout_logger_mt("bittorrent")),
              mAlertsLogger(spdlog::stdout_logger_mt("alerts")),
              mIsRunning(true) {

        configure(pSettings);
        mSession = std::make_shared<libtorrent::session>(mSettingsPack, libtorrent::session::add_default_plugins);
        load_torrent_files();

        mThreads.emplace_back(&Service::check_save_resume_data_handler, this);
        mThreads.emplace_back(&Service::consume_alerts_handler, this);
        mThreads.emplace_back(&Service::progress_handler, this);
    }

    Service::~Service() {
        mIsRunning = false;
        mCv.notify_all();
        for (auto &thread : mThreads) {
            thread.join();
        }
    }

    void Service::check_save_resume_data_handler() {
        mLogger->debug("operation=check_save_resume_data_handler, message='Initializing handler'");
        while (!wait_for_abort(mSettings.session_save)) {
            if (!mTorrents.empty()) {
                std::lock_guard<std::mutex> lock(mTorrentsMutex);
                for (auto &torrent: mTorrents) {
                    torrent->check_save_resume_data();
                }
            }
        }

        mLogger->debug("operation=check_save_resume_data_handler, message='Terminating handler'");
    }

    void Service::consume_alerts_handler() {
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

    void Service::handle_save_resume_data(const libtorrent::save_resume_data_alert *pAlert) {
        auto infoHash = get_info_hash(pAlert->handle.info_hash());
        mLogger->debug("operation=handle_save_resume_data, message='Saving resume data', infoHash={}", infoHash);

        auto buffer = libtorrent::write_resume_data_buf(pAlert->params);
        std::ofstream of(get_fast_resume_file(infoHash), std::ios::binary);
        of.unsetf(std::ios::skipws);
        of.write(buffer.data(), int(buffer.size()));
    }

    void Service::handle_metadata_received(const libtorrent::metadata_received_alert *pAlert) {
        auto torrentFile = pAlert->handle.torrent_file();
        auto infoHash = get_info_hash(torrentFile->info_hash());

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

    void Service::handle_state_changed(const libtorrent::state_changed_alert *pAlert) {
        if (pAlert->state == libtorrent::torrent_status::downloading) {
            auto infoHash = get_info_hash(pAlert->handle.info_hash());
            try {
                get_torrent(infoHash)->check_available_space();
            } catch (const std::exception &e) {
                mLogger->error("operation=handle_state_changed, message='Failed handling state change', what='{}'",
                               e.what());
            }
        }
    }

    void Service::progress_handler() {
        mLogger->debug("operation=progress_handler, message='Initializing handler'");
        std::chrono::seconds progressPeriodicity(1);
        while (!wait_for_abort(progressPeriodicity)) {
            // TODO
        }

        mLogger->debug("operation=progress_handler, message='Terminating handler'");
    }

    void Service::reconfigure(const Settings &pSettings, bool pReset) {
        mLogger->debug("operation=reconfigure, message='Reconfiguring service', reset={}", pReset);
        std::lock_guard<std::mutex> lock(mServiceMutex);

        configure(pSettings);
        mSession->apply_settings(mSettingsPack);

        if (pReset) {
            mLogger->debug("operation=reconfigure, message='Resetting torrents'");
            remove_torrents();
            load_torrent_files();
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
            pTorrentParams.save_path = mSettings.download_path;
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

        mTorrents.emplace_back(std::make_shared<Torrent>(weak_from_this(), handle, pInfoHash));
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

        auto infoHash = get_info_hash(torrentParams.info_hash);
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);

        if (pSaveMagnet) {
            std::ofstream of(get_magnet_file(infoHash), std::ios::binary);
            of << Magnet{pMagnet, pDownload};
        }

        return infoHash;
    }

    std::string Service::add_magnet(const std::string &pMagnet, bool pDownload) {
        return add_magnet(pMagnet, pDownload, true);
    }

    std::string Service::add_torrent_data(const std::vector<char> &pData, bool pDownload) {
        mLogger->debug("operation=add_torrent_data, message='Adding torrent data', download={}", pDownload);
        libtorrent::error_code errorCode;
        libtorrent::add_torrent_params torrentParams;
        torrentParams.ti = std::make_shared<libtorrent::torrent_info>(pData.data(), int(pData.size()), errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_torrent_data, message='Failed adding torrent data: {}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.ti->info_hash());
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);
        std::ofstream of(get_torrent_file(infoHash), std::ios::binary);
        of.write(pData.data(), int(pData.size()));

        return infoHash;
    }

    std::string Service::add_torrent_file(const std::string &pFile, bool pDownload) {
        mLogger->debug("operation=add_torrent_file, message='Adding torrent file', download={}", pDownload);
        libtorrent::error_code errorCode;
        libtorrent::add_torrent_params torrentParams;
        torrentParams.ti = std::make_shared<libtorrent::torrent_info>(pFile, errorCode);
        if (errorCode.failed()) {
            mLogger->error("operation=add_torrent_file, message='Failed adding torrent file: {}'", errorCode.message());
            throw LoadTorrentException(errorCode.message());
        }

        auto infoHash = get_info_hash(torrentParams.ti->info_hash());
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        add_torrent_with_params(torrentParams, infoHash, false, pDownload);

        auto destPath = get_torrent_file(infoHash);
        if (!std::experimental::filesystem::equivalent(pFile, destPath)) {
            std::experimental::filesystem::copy_file(pFile, destPath);
        }

        return infoHash;
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

        auto infoHash = get_info_hash(torrentParams.info_hash);
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        add_torrent_with_params(torrentParams, infoHash, true, true);
    }

    void Service::load_torrent_files() {
        mLogger->debug("operation=load_torrent_files, message='Loading torrent files'");
        std::vector<std::string> fastResumeFiles;
        std::vector<std::string> torrentFiles;
        std::vector<std::string> magnetFiles;

        for (auto &p : std::experimental::filesystem::directory_iterator(mSettings.torrents_path)) {
            if (std::experimental::filesystem::is_regular_file(p.path())) {
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
                std::experimental::filesystem::remove(f);
            }
        }

        for (auto &f : torrentFiles) {
            try {
                add_torrent_file(f, false);
            } catch (const LoadTorrentException &e) {
                mLogger->error("operation=load_torrent_files, message='Failed loading torrent', what='{}', file='{}'",
                               e.what(), f);
                std::experimental::filesystem::remove(f);
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
                std::experimental::filesystem::remove(f);
            }
        }

        for (auto &p : std::experimental::filesystem::directory_iterator(mSettings.download_path)) {
            if (p.path().extension() == EXT_PARTS && std::experimental::filesystem::is_regular_file(p.path())) {
                auto infoHash = p.path().stem().string();
                if (!has_torrent(infoHash)) {
                    mLogger->debug("operation=load_torrent_files, message='Cleaning stale parts', infoHash={}",
                                   infoHash);
                    std::experimental::filesystem::remove(p.path());
                }
            }
        }
    }

    std::vector<std::shared_ptr<Torrent>>::iterator Service::find_torrent(const std::string &pInfoHash,
                                                                          bool pMustFind) {
        mLogger->debug("operation=find_torrent, infoHash={}", pInfoHash);
        auto torrent = std::find_if(
                mTorrents.begin(), mTorrents.end(),
                [&pInfoHash](const std::shared_ptr<Torrent> &t) { return t->get_info_hash() == pInfoHash; });

        if (pMustFind && torrent == mTorrents.end()) {
            mLogger->error("operation=get_torrent, message='Unable to find torrent', infoHash={}", pInfoHash);
            throw InvalidInfoHashException("No such info hash");
        }

        return torrent;
    }

    bool Service::has_torrent(const std::string &pInfoHash) {
        return find_torrent(pInfoHash, false) != mTorrents.end();
    }

    std::shared_ptr<Torrent> Service::get_torrent(const std::string &pInfoHash) {
        mLogger->debug("operation=get_torrent, infoHash={}", pInfoHash);
        std::lock_guard<std::mutex> lock(mTorrentsMutex);
        return *find_torrent(pInfoHash);
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

    bool Service::wait_for_abort(int &pSeconds) {
        std::chrono::seconds seconds(pSeconds);
        return wait_for_abort(seconds);
    }

    bool Service::wait_for_abort(std::chrono::seconds &pSeconds) {
        auto until = std::chrono::steady_clock::now() + pSeconds;
        std::unique_lock<std::mutex> lock(mCvMutex);
        return mCv.wait_until(lock, until, [this] { return !mIsRunning.load(); });
    }

}