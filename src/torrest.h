#ifndef TORREST_TORREST_H
#define TORREST_TORREST_H

#include <atomic>
#include <mutex>

#include "spdlog/spdlog.h"

#include "settings/settings.h"
#include "bittorrent/service.h"

namespace torrest {

    class Torrest {
    public:
        static void initialize(const std::string &pSettingsPath);

        static void destroy();

        static Torrest &get_instance() {
            assert(mInstance);
            return *mInstance;
        }

        bool is_running() const {
            return mIsRunning.load();
        }

        void shutdown();

        std::string dump_settings() const;

        std::int64_t get_buffer_size() const;

        spdlog::level::level_enum get_api_log_level() const;

        void update_settings(const settings::Settings &pSettings, bool pReset);

        std::shared_ptr<bittorrent::Service> get_service() const {
            return mService;
        }

        Torrest(Torrest const &) = delete;

        void operator=(Torrest const &) = delete;

    private:
        explicit Torrest(std::string pSettingsPath);

        static void shutdown_signal(int signum);

        static Torrest *mInstance;
        mutable std::mutex mSettingsMutex;
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<bittorrent::Service> mService;
        std::string mSettingsPath;
        settings::Settings mSettings;
        std::atomic<bool> mIsRunning;
    };

}

#endif //TORREST_TORREST_H
