#include "logger.h"

#include "spdlog/sinks/stdout_sinks.h"

namespace torrest {

    std::shared_ptr<ApiLogger> ApiLogger::get_instance() {
        static std::shared_ptr<ApiLogger> instance{new ApiLogger};
        return instance;
    }

    ApiLogger::ApiLogger() : mLogger(spdlog::stdout_logger_mt("api")) {}

    void ApiLogger::log(v_uint32 pPriority, const std::string &pTag, const std::string &pMessage) {
        mLogger->log(get_associated_level(pPriority), "tag={}, message='{}'", pTag, pMessage);
    }

    bool ApiLogger::isLogPriorityEnabled(v_uint32 pPriority) {
        return get_associated_level(pPriority) >= mLogger->level();
    }

    std::shared_ptr<spdlog::logger> ApiLogger::get_logger() {
        return mLogger;
    }

    spdlog::level::level_enum ApiLogger::get_associated_level(v_uint32 pPriority) {
        spdlog::level::level_enum level;

        switch (pPriority) {
            case PRIORITY_V:
                level = spdlog::level::level_enum::trace;
                break;
            case PRIORITY_D:
                level = spdlog::level::level_enum::debug;
                break;
            case PRIORITY_I:
                level = spdlog::level::level_enum::info;
                break;
            case PRIORITY_W:
                level = spdlog::level::level_enum::warn;
                break;
            case PRIORITY_E:
                level = spdlog::level::level_enum::err;
                break;
            default:
                level = spdlog::level::level_enum::debug;
        }

        return level;
    }
}