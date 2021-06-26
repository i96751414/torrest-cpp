#ifndef TORREST_TORREST_H
#define TORREST_TORREST_H

#include <csignal>
#include <atomic>

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
            return mIsRunning.load();
        }

        std::string dump_settings() {
            return mSettings.dump();
        }

        void update_settings(const Settings &pSettings, bool pReset) {
            mLogger->debug("operation=update_settings, message='Updating settings'");
            mService->reconfigure(pSettings, pReset);
            mSettings = pSettings;
            mSettings.save(mSettingsPath);
        }

        std::shared_ptr<Service> get_service() {
            return mService;
        }

        Torrest(Torrest const &) = delete;

        void operator=(Torrest const &) = delete;

    private:
        explicit Torrest(std::string pSettingsPath, const Settings &pSettings)
                : mLogger(spdlog::stdout_logger_mt("torrest")),
                  mService(std::make_shared<Service>(pSettings)),
                  mSettingsPath(std::move(pSettingsPath)),
                  mSettings(pSettings),
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
        std::shared_ptr<Service> mService;
        std::string mSettingsPath;
        Settings mSettings;
        std::atomic<bool> mIsRunning;
    };

    Torrest *Torrest::mInstance = nullptr;
}

#endif //TORREST_TORREST_H
