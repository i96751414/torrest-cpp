#ifndef TORREST_SERVICE_SETTINGS_H
#define TORREST_SERVICE_SETTINGS_H

#include <mutex>

#include "settings/settings.h"

#define SETTING(mtx, type, name)                    \
    private:                                        \
        type name;                                  \
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

    struct ServiceSettings {

    SETTING(mMutex, bool, limit_after_buffering)
    SETTING(mMutex, int, max_download_rate)
    SETTING(mMutex, int, max_upload_rate)
    SETTING(mMutex, std::string, download_path)
    SETTING(mMutex, std::string, torrents_path)
    SETTING(mMutex, bool, check_available_space)
    SETTING(mMutex, int, session_save)
    SETTING(mMutex, int, seed_time_limit)
    SETTING(mMutex, int, seed_time_ratio_limit)
    SETTING(mMutex, int, share_ratio_limit)
#if !TORREST_LEGACY_READ_PIECE
    SETTING(mMutex, int, piece_expiration)
#endif
    SETTING(mMutex, int, piece_wait_timeout)

    public:
        explicit ServiceSettings(const settings::Settings &pSettings)
            : limit_after_buffering(pSettings.limit_after_buffering),
              max_download_rate(pSettings.max_download_rate),
              max_upload_rate(pSettings.max_upload_rate),
              download_path(pSettings.download_path),
              torrents_path(pSettings.torrents_path),
              check_available_space(pSettings.check_available_space),
              session_save(pSettings.session_save),
              seed_time_limit(pSettings.seed_time_limit),
              seed_time_ratio_limit(pSettings.seed_time_ratio_limit),
              share_ratio_limit(pSettings.share_ratio_limit),
#if !TORREST_LEGACY_READ_PIECE
              piece_expiration(pSettings.piece_expiration),
#endif
              piece_wait_timeout(pSettings.piece_wait_timeout) {}

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
#if !TORREST_LEGACY_READ_PIECE
            piece_expiration = pSettings.piece_expiration;
#endif
            piece_wait_timeout = pSettings.piece_wait_timeout;
        }

    private:
        mutable std::mutex mMutex;
    };

}}

#endif //TORREST_SERVICE_SETTINGS_H
