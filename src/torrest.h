#ifndef TORREST_TORREST_H
#define TORREST_TORREST_H

#include <csignal>

#include "spdlog/spdlog.h"

#include "settings/settings.h"
#include "bittorrent/service.h"

namespace torrest {
    class Torrest {
    public:
        static void initialize(const std::string &pSettingsPath, const Settings &pSettings) {
            static Torrest instance(pSettingsPath, pSettings);
            mInstance = &instance;
        }

        static Torrest &get_instance() {
            return *mInstance;
        }

        bool is_running() const {
            return mIsRunning;
        }

        std::string dump_settings() {
            return mSettings.dump();
        }

        Torrest(Torrest const &) = delete;

        void operator=(Torrest const &) = delete;

    private:
        explicit Torrest(std::string pSettingsPath, const Settings &pSettings)
                : mLogger(spdlog::stdout_logger_mt("torrest")),
                  mSettingsPath(std::move(pSettingsPath)),
                  mSettings(pSettings),
                  mService(pSettings),
                  mIsRunning(true) {
            std::signal(SIGINT, shutdown_signal);
            std::signal(SIGKILL, shutdown_signal);
            std::signal(SIGTERM, shutdown_signal);
        };

        static void shutdown_signal(int signum) {
            mInstance->mLogger->debug(
                    "operation=shutdown_signal, message='Received shutdown signal', signum={}", signum);
            mInstance->mIsRunning = false;
        }

        static Torrest *mInstance;

        std::shared_ptr<spdlog::logger> mLogger;
        std::string mSettingsPath;
        Settings mSettings;
        Service mService;
        bool mIsRunning;
    };

    Torrest *Torrest::mInstance = nullptr;
}

#endif //TORREST_TORREST_H
