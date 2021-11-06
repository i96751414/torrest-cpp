#ifndef TORREST_LOGGER_H
#define TORREST_LOGGER_H

#include "oatpp/core/base/Environment.hpp"
#include "spdlog/spdlog.h"

namespace torrest { namespace api {

    class ApiLogger : public oatpp::base::Logger {
    public:
        static std::shared_ptr<ApiLogger> get_instance();

        void log(v_uint32 pPriority, const std::string &pTag, const std::string &pMessage) override;

        bool isLogPriorityEnabled(v_uint32 pPriority) override;

        std::shared_ptr<spdlog::logger> get_logger();

        ApiLogger(ApiLogger const &) = delete;

        void operator=(ApiLogger const &) = delete;

    private:
        ApiLogger();

        static spdlog::level::level_enum get_associated_level(v_uint32 pPriority);

        std::shared_ptr<spdlog::logger> mLogger;
    };

}}

#endif //TORREST_LOGGER_H
