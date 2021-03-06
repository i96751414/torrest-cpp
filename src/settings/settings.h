#ifndef TORREST_SETTINGS_H
#define TORREST_SETTINGS_H

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace nlohmann {

    template<class T>
    void to_json(nlohmann::json &pJson, const std::shared_ptr<T> &pValue) {
        if (pValue) {
            pJson = *pValue;
        } else {
            pJson = nullptr;
        }
    }

    template<class T>
    void from_json(const nlohmann::json &pJson, std::shared_ptr<T> &pValue) {
        pValue = pJson.is_null() ? nullptr : std::make_shared<T>(pJson.get<T>());
    }

}

namespace torrest { namespace settings {

    enum UserAgent {
        ua_torrest_default,
        ua_libtorrent,
        ua_libtorrent_rasterbar_1_1_0,
        ua_bittorrent_7_5_0,
        ua_bittorrent_7_4_3,
        ua_utorrent_3_4_9,
        ua_utorrent_3_2_0,
        ua_utorrent_2_2_1,
        ua_transmission_2_92,
        ua_deluge_1_3_6_0,
        ua_deluge_1_3_12_0,
        ua_vuze_5_7_3_0,
        ua_num_values
    };

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

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ProxySettings,
                                       type,
                                       port,
                                       hostname,
                                       username,
                                       password)

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
        UserAgent user_agent = ua_torrest_default;
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
        spdlog::level::level_enum service_log_level = spdlog::level::info;
        spdlog::level::level_enum alerts_log_level = spdlog::level::critical;
        spdlog::level::level_enum api_log_level = spdlog::level::err;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Settings,
                                       listen_port,
                                       listen_interfaces,
                                       outgoing_interfaces,
                                       disable_dht,
                                       disable_upnp,
                                       disable_natpmp,
                                       disable_lsd,
                                       download_path,
                                       torrents_path,
                                       user_agent,
                                       session_save,
                                       tuned_storage,
                                       check_available_space,
                                       connections_limit,
                                       limit_after_buffering,
                                       max_download_rate,
                                       max_upload_rate,
                                       share_ratio_limit,
                                       seed_time_ratio_limit,
                                       seed_time_limit,
                                       active_downloads_limit,
                                       active_seeds_limit,
                                       active_checking_limit,
                                       active_dht_limit,
                                       active_tracker_limit,
                                       active_lsd_limit,
                                       active_limit,
                                       encryption_policy,
                                       proxy,
                                       buffer_size,
                                       piece_wait_timeout,
                                       service_log_level,
                                       alerts_log_level,
                                       api_log_level)

        static Settings load(const std::string &pPath);

        static Settings parse(const std::string &pJson);

        void save(const std::string &pPath) const;

        std::string dump() const;

        void validate() const;
    };

    std::string get_user_agent(UserAgent pUserAgent);

}}

#endif //TORREST_SETTINGS_H
