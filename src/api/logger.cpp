#include "logger.h"

#include <utility>

#include "utils/log.h"

namespace torrest { namespace api {

    ApiLogger::ApiLogger(std::shared_ptr<spdlog::logger> pLogger)
        : mLogger(std::move(pLogger)) {}

    void ApiLogger::log(v_uint32 pPriority, const std::string &pTag, const std::string &pMessage) {
        mLogger->log(get_associated_level(pPriority), "tag={}, message='{}'", pTag, pMessage);
    }

    bool ApiLogger::isLogPriorityEnabled(v_uint32 pPriority) {
        return get_associated_level(pPriority) >= mLogger->level();
    }

    spdlog::level::level_enum ApiLogger::get_associated_level(v_uint32 pPriority) {
        spdlog::level::level_enum level;

        switch (pPriority) {
            case PRIORITY_V:
                level = spdlog::level::trace;
                break;
            case PRIORITY_D:
                level = spdlog::level::debug;
                break;
            case PRIORITY_I:
                level = spdlog::level::info;
                break;
            case PRIORITY_W:
                level = spdlog::level::warn;
                break;
            case PRIORITY_E:
                level = spdlog::level::err;
                break;
            default:
                level = spdlog::level::debug;
        }

        return level;
    }

}}
