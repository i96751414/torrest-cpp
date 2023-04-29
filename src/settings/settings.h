#ifndef TORREST_SETTINGS_H
#define TORREST_SETTINGS_H

#include "spdlog/spdlog.h"

namespace torrest { namespace settings {

    enum EncryptionPolicy {
        ep_enabled,
        ep_disabled,
        ep_forced,
        ep_num_values
    };

    enum ProxyType {
        pt_none,
        pt_socks4,
        pt_socks5,
        pt_socks5_password,
        pt_http,
        pt_http_password,
        pt_i2psam,
        pt_num_values
    };

    struct ProxySettings {
        ProxyType type = pt_none;
        int port = 0;
        std::string hostname;
        std::string username;
        std::string password;

        void validate() const;
    };

    struct Settings {
        int listen_port = 6889;
        std::string listen_interfaces;
        std::string outgoing_interfaces;
        bool disable_dht = false;
        bool disable_upnp = false;
        bool disable_natpmp = false;
        bool disable_lsd = false;
        std::string download_path = "downloads";
        std::string torrents_path = "downloads/torrents";
        std::string user_agent;
        int session_save = 30;
        bool tuned_storage = false;
        bool check_available_space = true;
        int connections_limit = 0;
        bool limit_after_buffering = false;
        int max_download_rate = 0;
        int max_upload_rate = 0;
        int share_ratio_limit = 0;
        int seed_time_ratio_limit = 0;
        int seed_time_limit = 0;
        int active_downloads_limit = 3;
        int active_seeds_limit = 5;
        int active_checking_limit = 1;
        int active_dht_limit = 88;
        int active_tracker_limit = 1600;
        int active_lsd_limit = 60;
        int active_limit = 500;
        EncryptionPolicy encryption_policy = ep_enabled;
        std::shared_ptr<ProxySettings> proxy = nullptr;
        std::int64_t buffer_size = 20 * 1024 * 1024;
        int piece_wait_timeout = 60;
#if !TORREST_LEGACY_READ_PIECE
        int piece_expiration = 5;
#endif
        spdlog::level::level_enum service_log_level = spdlog::level::info;
        spdlog::level::level_enum alerts_log_level = spdlog::level::critical;
        spdlog::level::level_enum api_log_level = spdlog::level::err;

        static Settings load(const std::string &pPath);

        static Settings parse(const std::string &pJson);

        void save(const std::string &pPath) const;

        std::string dump() const;

        void validate() const;
    };

}}

#endif //TORREST_SETTINGS_H
