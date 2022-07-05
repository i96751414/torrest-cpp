#include "logger_interceptor.h"

#include "spdlog/spdlog.h"

#if TORREST_EXTENDED_CONNECTIONS
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#endif

#include "logger.h"

namespace torrest { namespace api {

    std::shared_ptr<LoggerRequestInterceptor::OutgoingResponse>
    LoggerRequestInterceptor::intercept(const std::shared_ptr<IncomingRequest> &pRequest) {
        oatpp::Int64 startTime(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        pRequest->putBundleData("startTime", startTime);
        return nullptr;
    }

    std::shared_ptr<LoggerResponseInterceptor::OutgoingResponse>
    LoggerResponseInterceptor::intercept(const std::shared_ptr<IncomingRequest> &pRequest,
                                         const std::shared_ptr<OutgoingResponse> &pResponse) {
        oatpp::Int64 endTime(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        auto duration = endTime - pRequest->getBundleData<oatpp::Int64>("startTime");

        spdlog::level::level_enum level;
        auto statusCode = pResponse->getStatus().code;

        if (statusCode >= 500) {
            level = spdlog::level::err;
        } else if (statusCode >= 400) {
            level = spdlog::level::warn;
        } else {
            level = spdlog::level::info;
        }

        auto &startingLine = pRequest->getStartingLine();
        ApiLogger::get_instance()->get_logger()->log(
                level, "operation=intercept"
                       ", method={}"
#if TORREST_EXTENDED_CONNECTIONS
                       ", address={}"
#endif
                       ", uri={}"
                       ", elapsedTime={}"
                       ", statusCode={}",
                startingLine.method.std_str(),
#if TORREST_EXTENDED_CONNECTIONS
                pRequest->getConnection()->getInputStreamContext().getProperties().get(
                        oatpp::network::tcp::server::ConnectionProvider::ExtendedConnection::PROPERTY_PEER_ADDRESS)->c_str(),
#endif
                startingLine.path.std_str(), duration, statusCode);

        return pResponse;
    }

}}
