#ifndef TORREST_LOGGER_INTERCEPTOR_H
#define TORREST_LOGGER_INTERCEPTOR_H

#include "oatpp/web/server/interceptor/ResponseInterceptor.hpp"

namespace torrest { namespace api {

    class LoggerInterceptor : public oatpp::web::server::interceptor::ResponseInterceptor {
    public:
        typedef oatpp::web::protocol::http::incoming::Request IncomingRequest;
        typedef oatpp::web::protocol::http::outgoing::Response OutgoingResponse;

        std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest> &request,
                                                    const std::shared_ptr<OutgoingResponse> &response) override;
    };

}}

#endif //TORREST_LOGGER_INTERCEPTOR_H
