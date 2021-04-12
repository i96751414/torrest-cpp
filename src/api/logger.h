#ifndef TORREST_LOGGER_H
#define TORREST_LOGGER_H

#include "oatpp/core/base/Environment.hpp"
#include "spdlog/spdlog.h"

namespace torrest {

    class ApiLogger : public oatpp::base::Logger {
    public:
        ApiLogger();

        void log(v_uint32 priority, const std::string &tag, const std::string &message) override;

        bool isLogPriorityEnabled(v_uint32 priority) override;

        void set_log_level(spdlog::level::level_enum level);

    private:
        static spdlog::level::level_enum get_associated_level(v_uint32 priority);

        std::shared_ptr<spdlog::logger> mLogger;
    };

}

#endif //TORREST_LOGGER_H
