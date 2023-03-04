#ifndef TORREST_LOGGER_H
#define TORREST_LOGGER_H

#include "oatpp/core/base/Environment.hpp"
#include "spdlog/spdlog.h"

namespace torrest { namespace api {

    class ApiLogger : public oatpp::base::Logger {
    public:
        void log(v_uint32 pPriority, const std::string &pTag, const std::string &pMessage) override;

        bool isLogPriorityEnabled(v_uint32 pPriority) override;

        explicit ApiLogger(std::shared_ptr<spdlog::logger> pLogger);

    private:
        static spdlog::level::level_enum get_associated_level(v_uint32 pPriority);

        std::shared_ptr<spdlog::logger> mLogger;
    };

}}

#endif //TORREST_LOGGER_H
