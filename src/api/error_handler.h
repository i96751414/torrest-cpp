#ifndef TORREST_ERROR_HANDLER_H
#define TORREST_ERROR_HANDLER_H

#include "oatpp/web/server/handler/ErrorHandler.hpp"
#include "oatpp/core/data/mapping/ObjectMapper.hpp"

namespace torrest { namespace api {

    class ErrorHandler : public oatpp::web::server::handler::ErrorHandler {
    public:
        explicit ErrorHandler(std::shared_ptr<oatpp::data::mapping::ObjectMapper> pObjectMapper);

        std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
        handleError(const oatpp::web::protocol::http::Status &pStatus,
                    const oatpp::String &pMessage,
                    const Headers &pHeaders) override;

    private:
        std::shared_ptr<oatpp::data::mapping::ObjectMapper> mObjectMapper;
    };

}}

#endif //TORREST_ERROR_HANDLER_H
