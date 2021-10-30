#include "error_handler.h"

#include "oatpp/web/protocol/http/outgoing/ResponseFactory.hpp"
#include "nlohmann/json.hpp"
#include "range_parser/range_parser.hpp"

#include "api/dto/error_response.h"
#include "utils/validation.h"
#include "bittorrent/exceptions.h"

namespace torrest { namespace api {

    ErrorHandler::ErrorHandler(std::shared_ptr<oatpp::data::mapping::ObjectMapper> pObjectMapper)
            : mObjectMapper(std::move(pObjectMapper)) {}

    std::shared_ptr<oatpp::web::protocol::http::outgoing::Response>
    ErrorHandler::handleError(const oatpp::web::protocol::http::Status &pStatus,
                              const oatpp::String &pMessage,
                              const Headers &pHeaders) {

        auto status = pStatus;
        auto message = pMessage;

        if (status == oatpp::web::protocol::http::Status::CODE_500) {
            try {
                // We can throw for HTTP 500 errors only as per oatpp/web/server/HttpProcessor.cpp
                throw; // lgtm [cpp/rethrow-no-exception]
            } catch (const ValidationException &e) {
                status = oatpp::web::protocol::http::Status::CODE_400;
            } catch (const nlohmann::json::exception &e) {
                status = oatpp::web::protocol::http::Status::CODE_400;
                message = "Invalid json object";
            } catch (const bittorrent::InvalidInfoHashException &e) {
                status = oatpp::web::protocol::http::Status::CODE_404;
            } catch (const bittorrent::InvalidFileIndexException &e) {
                status = oatpp::web::protocol::http::Status::CODE_404;
            } catch (const range_parser::RangeException &e) {
                status = oatpp::web::protocol::http::Status::CODE_416;
            } catch (...) {
                // use the default status and message
            }
        }

        auto response = oatpp::web::protocol::http::outgoing::ResponseFactory::createResponse(
                status, ErrorResponse::create(message), mObjectMapper);

        for (const auto &header : pHeaders.getAll()) {
            response->putHeader(header.first.toString(), header.second.toString());
        }

        return response;
    }

}}
