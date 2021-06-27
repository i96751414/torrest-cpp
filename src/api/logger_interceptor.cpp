#include "logger_interceptor.h"

#include "spdlog/spdlog.h"

#include "logger.h"

namespace torrest { namespace api {

    std::shared_ptr<LoggerInterceptor::OutgoingResponse>
    LoggerInterceptor::intercept(const std::shared_ptr<IncomingRequest> &request,
                                 const std::shared_ptr<OutgoingResponse> &response) {
        spdlog::level::level_enum level;
        auto statusCode = response->getStatus().code;

        if (statusCode >= 500) {
            level = spdlog::level::err;
        } else if (statusCode >= 400) {
            level = spdlog::level::warn;
        } else {
            level = spdlog::level::info;
        }

        auto &startingLine = request->getStartingLine();
        ApiLogger::get_instance()->get_logger()->log(
                level, "operation=intercept, method={}, uri={}, statusCode={}",
                startingLine.method.std_str(), startingLine.path.std_str(), statusCode);

        return response;
    }

}}
