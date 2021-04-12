#ifndef TORREST_ERROR_HANDLER_H
#define TORREST_ERROR_HANDLER_H

#include "oatpp/web/server/handler/ErrorHandler.hpp"
#include "oatpp/web/protocol/http/outgoing/ResponseFactory.hpp"
#include "nlohmann/json.hpp"

#include "api/dto/error_response.h"
#include "utils/validation.h"

namespace torrest {

    class ErrorHandler : public oatpp::web::server::handler::ErrorHandler {
    public:
        explicit ErrorHandler(std::shared_ptr<oatpp::data::mapping::ObjectMapper> objectMapper)
                : mObjectMapper(std::move(objectMapper)) {};

        std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
        handleError(const oatpp::web::protocol::http::Status &pStatus,
                    const oatpp::String &pMessage,
                    const Headers &pHeaders) override {

            auto status = pStatus;
            auto message = pMessage;

            try {
                throw;
            } catch (const validation_exception &e) {
                status = oatpp::web::protocol::http::Status::CODE_400;
            } catch (const nlohmann::json::exception &e) {
                status = oatpp::web::protocol::http::Status::CODE_400;
                message = "Invalid json object";
            } catch (...) {
                // use the default status and message
            }

            auto errorResponse = ErrorResponse::createShared();
            errorResponse->error = message;

            auto response = oatpp::web::protocol::http::outgoing::ResponseFactory::createResponse(
                    status, errorResponse, mObjectMapper);

            for (const auto &header : pHeaders.getAll()) {
                response->putHeader(header.first.toString(), header.second.toString());
            }

            return response;
        }

    private:
        std::shared_ptr<oatpp::data::mapping::ObjectMapper> mObjectMapper;
    };

}

#endif //TORREST_ERROR_HANDLER_H
