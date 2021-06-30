#ifndef TORREST_SERVICE_SETTINGS_H
#define TORREST_SERVICE_SETTINGS_H

#include <mutex>

#define SETTING(type, name, def)                    \
    private:                                        \
        type name = def;                            \
    public:                                         \
        type get_##name() const {                   \
            std::lock_guard<std::mutex> l(mMutex);  \
            return name;                            \
        }                                           \
        void set_##name(const type &pValue) {       \
            std::lock_guard<std::mutex> l(mMutex);  \
            name = pValue;                          \
        }
    

namespace torrest { namespace bittorrent {

    class ServiceSettings {

    SETTING(bool, limit_after_buffering, false)
    SETTING(int, max_download_rate, 0)
    SETTING(int, max_upload_rate, 0)
    SETTING(std::string, download_path, "downloads")
    SETTING(std::string, torrents_path, "downloads/torrents")
    SETTING(bool, check_available_space, true)
    SETTING(int, session_save, 30)
    SETTING(int, seed_time_limit, 0)
    SETTING(int, seed_time_ratio_limit, 0)
    SETTING(int, share_ratio_limit, 0)

    public:
        void update(const Settings &pSettings) {
            std::lock_guard<std::mutex> l(mMutex);
            limit_after_buffering = pSettings.limit_after_buffering;
            max_download_rate = pSettings.max_download_rate;
            max_upload_rate = pSettings.max_upload_rate;
            download_path = pSettings.download_path;
            torrents_path = pSettings.torrents_path;
            check_available_space = pSettings.check_available_space;
            session_save = pSettings.session_save;
            seed_time_limit = pSettings.seed_time_limit;
            seed_time_ratio_limit = pSettings.seed_time_ratio_limit;
            share_ratio_limit = pSettings.share_ratio_limit;
        }

    private:
        mutable std::mutex mMutex;
    };

}}

#endif //TORREST_SERVICE_SETTINGS_H
