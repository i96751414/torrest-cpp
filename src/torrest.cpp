#include "torrest.h"

#include <csignal>

#include "boost/filesystem.hpp"
#include "spdlog/sinks/stdout_sinks.h"

namespace torrest {

    Torrest *Torrest::mInstance = nullptr;

    Torrest::Torrest(std::string pSettingsPath)
            : mLogger(spdlog::stdout_logger_mt("torrest")),
              mSettingsPath(std::move(pSettingsPath)),
              mIsRunning(true) {

        torrest::settings::Settings settings;
        if (boost::filesystem::exists(mSettingsPath)) {
            mLogger->debug("operation=initialize, message='Loading settings file', settingsPath='{}'", mSettingsPath);
            settings = torrest::settings::Settings::load(mSettingsPath);
            settings.validate();
        } else {
            mLogger->debug("operation=initialize, message='Saving default settings file', settingsPath='{}'",
                           mSettingsPath);
            settings.save(mSettingsPath);
        }

        mSettings = settings;
        mService = std::make_shared<bittorrent::Service>(settings);
    }

    void Torrest::initialize(const std::string &pSettingsPath) {
        assert(mInstance == nullptr);
        mInstance = new Torrest(pSettingsPath);
#if !TORREST_LIBRARY
        std::atexit(destroy);
#endif
        std::signal(SIGINT, shutdown_signal);
        std::signal(SIGTERM, shutdown_signal);
#ifndef _WIN32
        std::signal(SIGKILL, shutdown_signal);
#endif
    }

    void Torrest::destroy() {
        delete mInstance;
        mInstance = nullptr;
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

    spdlog::level::level_enum Torrest::get_api_log_level() const {
        mLogger->trace("operation=get_api_log_level");
        std::lock_guard<std::mutex> lock(mSettingsMutex);
        return mSettings.api_log_level;
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