#include "settings.h"
#include <fstream>
#include <iomanip>

#include "libtorrent/version.hpp"

#include "utils/validation.h"
#include "version.h"

namespace torrest {
    Settings Settings::load(const std::string &pPath) {
        std::ifstream i(pPath);
        nlohmann::json j;
        i >> j;
        return j.get<Settings>();
    }

    Settings Settings::parse(const std::string &pJson) {
        nlohmann::json j = nlohmann::json::parse(pJson);
        return j.get<Settings>();
    }

    void Settings::save(const std::string &pPath) {
        std::ofstream o(pPath);
        nlohmann::json j = *this;
        o << std::setw(4) << j << std::endl;
    }

    std::string Settings::dump() {
        nlohmann::json j = *this;
        return j.dump(4);
    }

    void Settings::validate() const {
        VALIDATE(listen_port, GTE(0), LTE(65535))
        VALIDATE(download_path, NOT_EMPTY())
        VALIDATE(torrents_path, NOT_EMPTY())
        VALIDATE(user_agent, GTE(0), LT(ua_num_values))
        VALIDATE(session_save, GT(0))
        VALIDATE(max_download_rate, GTE(0))
        VALIDATE(max_upload_rate, GTE(0))
        VALIDATE(share_ratio_limit, GTE(0))
        VALIDATE(seed_time_ratio_limit, GTE(0))
        VALIDATE(seed_time_limit, GTE(0))
        VALIDATE(encryption_policy, GTE(0), LT(ep_num_values))
        VALIDATE(proxy_type, GTE(0), LT(pt_num_values))
        VALIDATE(proxy_port, GTE(0), LTE(65535))
        VALIDATE(piece_wait_timeout, GTE(0))
        VALIDATE(service_log_level, GTE(0), LT(spdlog::level::n_levels))
        VALIDATE(alert_log_level, GTE(0), LT(spdlog::level::n_levels))
        VALIDATE(api_log_level, GTE(0), LT(spdlog::level::n_levels))
    }

    std::string get_user_agent(UserAgent pUserAgent) {
        switch (pUserAgent) {
            case ua_libtorrent:
                return "libtorrent/" LIBTORRENT_VERSION;
            case ua_libtorrent_rasterbar_1_1_0:
                return "libtorrent (Rasterbar) 1.1.0";
            case ua_bittorrent_7_5_0:
                return "BitTorrent 7.5.0";
            case ua_bittorrent_7_4_3:
                return "BitTorrent 7.4.3";
            case ua_utorrent_3_4_9:
                return "µTorrent 3.4.9";
            case ua_utorrent_3_2_0:
                return "µTorrent 3.2.0";
            case ua_utorrent_2_2_1:
                return "µTorrent 2.2.1";
            case ua_transmission_2_92:
                return "Transmission 2.92";
            case ua_deluge_1_3_6_0:
                return "Deluge 1.3.6.0";
            case ua_deluge_1_3_12_0:
                return "Deluge 1.3.12.0";
            case ua_vuze_5_7_3_0:
                return "Vuze 5.7.3.0";
            default:
                return "torrest/" TORREST_VERSION " libtorrent/" LIBTORRENT_VERSION;
        }
    }
}