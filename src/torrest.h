#ifndef TORREST_TORREST_H
#define TORREST_TORREST_H

#include <csignal>
#include <atomic>
#include <mutex>

#include "spdlog/spdlog.h"

#include "settings/settings.h"
#include "bittorrent/service.h"

namespace torrest {

    class Torrest {
    public:
        static void initialize(const std::string &pSettingsPath, const settings::Settings &pSettings) {
            assert(mInstance == nullptr);
            static Torrest instance(pSettingsPath, pSettings);
            mInstance = &instance;
            std::signal(SIGINT, shutdown_signal);
            std::signal(SIGTERM, shutdown_signal);
#ifndef _WIN32
            std::signal(SIGKILL, shutdown_signal);
#endif
        }

        static Torrest &get_instance() {
            assert(mInstance);
            return *mInstance;
        }

        bool is_running() const {
            return mIsRunning.load();
        }

        void shutdown() {
            mLogger->trace("operation=shutdown");
            mIsRunning = false;
        }

        std::string dump_settings() const {
            std::lock_guard<std::mutex> lock(mMutex);
            return mSettings.dump();
        }

        std::int64_t get_buffer_size() const {
            std::lock_guard<std::mutex> lock(mMutex);
            return mSettings.buffer_size;
        }

        void update_settings(const settings::Settings &pSettings, bool pReset) {
            mLogger->debug("operation=update_settings, message='Updating settings'");
            std::lock_guard<std::mutex> lock(mMutex);
            mService->reconfigure(pSettings, pReset);
            mSettings = pSettings;
            mSettings.save(mSettingsPath);
        }

        std::shared_ptr<bittorrent::Service> get_service() const {
            return mService;
        }

        Torrest(Torrest const &) = delete;

        void operator=(Torrest const &) = delete;

    private:
        explicit Torrest(std::string pSettingsPath, const settings::Settings &pSettings)
                : mLogger(spdlog::stdout_logger_mt("torrest")),
                  mService(std::make_shared<bittorrent::Service>(pSettings)),
                  mSettingsPath(std::move(pSettingsPath)),
                  mSettings(pSettings),
                  mIsRunning(true) {}

        static void shutdown_signal(int signum) {
            assert(mInstance);
            mInstance->mLogger->debug(
                    "operation=shutdown_signal, message='Received shutdown signal', signum={}", signum);
            mInstance->shutdown();
        }

        static Torrest *mInstance;
        mutable std::mutex mMutex;
        std::shared_ptr<spdlog::logger> mLogger;
        std::shared_ptr<bittorrent::Service> mService;
        std::string mSettingsPath;
        settings::Settings mSettings;
        std::atomic<bool> mIsRunning;
    };

    Torrest *Torrest::mInstance = nullptr;

}

#endif //TORREST_TORREST_H
