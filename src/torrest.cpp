#include "torrest.h"

#include <csignal>

#include "spdlog/sinks/stdout_sinks.h"

namespace torrest {

    Torrest *Torrest::mInstance = nullptr;

    Torrest::Torrest(std::string pSettingsPath, const settings::Settings &pSettings)
            : mLogger(spdlog::stdout_logger_mt("torrest")),
              mService(std::make_shared<bittorrent::Service>(pSettings)),
              mSettingsPath(std::move(pSettingsPath)),
              mSettings(pSettings),
              mIsRunning(true) {}

    void Torrest::initialize(const std::string &pSettingsPath, const settings::Settings &pSettings) {
        assert(mInstance == nullptr);
        static Torrest instance(pSettingsPath, pSettings);
        mInstance = &instance;
        std::signal(SIGINT, shutdown_signal);
        std::signal(SIGTERM, shutdown_signal);
#ifndef _WIN32
        std::signal(SIGKILL, shutdown_signal);
#endif
    }

    void Torrest::shutdown() {
        mLogger->trace("operation=shutdown");
        mIsRunning = false;
    }

    std::string Torrest::dump_settings() const {
        mLogger->trace("operation=dump_settings");
        std::lock_guard<std::mutex> lock(mSettingsMutex);
        return mSettings.dump();
    }

    std::int64_t Torrest::get_buffer_size() const {
        mLogger->trace("operation=get_buffer_size");
        std::lock_guard<std::mutex> lock(mSettingsMutex);
        return mSettings.buffer_size;
    }

    void Torrest::update_settings(const settings::Settings &pSettings, bool pReset) {
        mLogger->debug("operation=update_settings, message='Updating settings'");
        std::lock_guard<std::mutex> lock(mSettingsMutex);
        mService->reconfigure(pSettings, pReset);
        mSettings = pSettings;
        mSettings.save(mSettingsPath);
    }

    void Torrest::shutdown_signal(int signum) {
        assert(mInstance);
        mInstance->mLogger->debug(
                "operation=shutdown_signal, message='Received shutdown signal', signum={}", signum);
        mInstance->shutdown();
    }

}