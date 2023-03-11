#ifndef TORREST_LOGGER_INTERCEPTOR_H
#define TORREST_LOGGER_INTERCEPTOR_H

#include "oatpp/web/server/interceptor/RequestInterceptor.hpp"
#include "oatpp/web/server/interceptor/ResponseInterceptor.hpp"

namespace torrest { namespace api {

    class LoggerRequestInterceptor : public oatpp::web::server::interceptor::RequestInterceptor {
    public:
        std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest> &pRequest) override;
    };

    class LoggerResponseInterceptor : public oatpp::web::server::interceptor::ResponseInterceptor {
    public:
        std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest> &pRequest,
                                                    const std::shared_ptr<OutgoingResponse> &pResponse) override;
    };

}}

#endif //TORREST_LOGGER_INTERCEPTOR_H
