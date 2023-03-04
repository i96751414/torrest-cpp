#include "torrest.h"

#include <csignal>

#include "boost/filesystem.hpp"

#include "utils/log.h"

namespace torrest {

    Torrest *Torrest::mInstance = nullptr;
    std::mutex Torrest::mInstanceMutex;

    Torrest::Torrest(std::string pSettingsPath)
            : mLogger(utils::create_logger("torrest")),
              mApiLogger(utils::create_logger("api")),
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
        mApiLogger->set_level(settings.api_log_level);
        mService = std::make_shared<bittorrent::Service>(settings);
    }

    void Torrest::initialize(const std::string &pSettingsPath) {
        std::lock_guard<std::mutex> lock(mInstanceMutex);
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
        std::lock_guard<std::mutex> lock(mInstanceMutex);
        delete mInstance;
        mInstance = nullptr;
    }

    void Torrest::try_shutdown() {
        std::lock_guard<std::mutex> lock(mInstanceMutex);
        if (mInstance != nullptr) {
            mInstance->shutdown();
        }
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
        mLogger->debug("operation=update_settings, message='Updating settings', reset={}", pReset);
        std::lock_guard<std::mutex> lock(mSettingsMutex);
        mService->reconfigure(pSettings, pReset);
        mApiLogger->set_level(pSettings.api_log_level);
        mSettings = pSettings;
        mSettings.save(mSettingsPath);
    }

    void Torrest::shutdown_signal(int signum) {
        std::lock_guard<std::mutex> lock(mInstanceMutex);
        if (mInstance != nullptr) {
            mInstance->mLogger->debug(
                    "operation=shutdown_signal, message='Received shutdown signal', signum={}", signum);
            mInstance->shutdown();
        }
    }

}