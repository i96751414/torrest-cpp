#ifndef TORREST_SERVICE_SETTINGS_H
#define TORREST_SERVICE_SETTINGS_H

#include <mutex>

#include "settings/settings.h"

#define SETTING(mtx, type, name, def)               \
    private:                                        \
        type name = def;                            \
    public:                                         \
        type get_##name() const {                   \
            std::lock_guard<std::mutex> l(mtx);     \
            return name;                            \
        }                                           \
        void set_##name(const type &pValue) {       \
            std::lock_guard<std::mutex> l(mtx);     \
            name = pValue;                          \
        }


namespace torrest { namespace bittorrent {

    class ServiceSettings {

    SETTING(mMutex, bool, limit_after_buffering, false)
    SETTING(mMutex, int, max_download_rate, 0)
    SETTING(mMutex, int, max_upload_rate, 0)
    SETTING(mMutex, std::string, download_path, "downloads")
    SETTING(mMutex, std::string, torrents_path, "downloads/torrents")
    SETTING(mMutex, bool, check_available_space, true)
    SETTING(mMutex, int, session_save, 30)
    SETTING(mMutex, int, seed_time_limit, 0)
    SETTING(mMutex, int, seed_time_ratio_limit, 0)
    SETTING(mMutex, int, share_ratio_limit, 0)
    SETTING(mMutex, int, piece_wait_timeout, 60)
#if !TORREST_LEGACY_READ_PIECE
    SETTING(mMutex, int, piece_expiration, 5)
#endif

    public:
        void update(const settings::Settings &pSettings) {
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
            piece_wait_timeout = pSettings.piece_wait_timeout;
#if !TORREST_LEGACY_READ_PIECE
            piece_expiration = pSettings.piece_expiration;
#endif
        }

    private:
        mutable std::mutex mMutex;
    };

}}

#endif //TORREST_SERVICE_SETTINGS_H
