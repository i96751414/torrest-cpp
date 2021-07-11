#ifndef TORREST_APP_COMPONENT_H
#define TORREST_APP_COMPONENT_H

#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/web/server/interceptor/AllowCorsGlobal.hpp"

#if TORREST_ENABLE_SWAGGER
#ifdef OATPP_SWAGGER_RES_PATH
#include "oatpp-swagger/Resources.hpp"
#endif
#include "oatpp-swagger/Model.hpp"
#endif //TORREST_ENABLE_SWAGGER

#include "error_handler.h"
#include "version.h"
#include "logger_interceptor.h"

namespace torrest { namespace api {
    /**
     *  Class which creates and holds Application components and registers components in oatpp::base::Environment.
     *  Order of components initialization is from top to bottom.
     */
    class AppComponent {
    public:
        explicit AppComponent(const uint16_t pPort) : mPort(pPort) {}

    private:
        uint16_t mPort;

        // Create ObjectMapper component to serialize/deserialize DTOs in Controller's API
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)
        (oatpp::parser::json::mapping::ObjectMapper::createShared());

        // Create ConnectionProvider component which listens on the port
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)
        (oatpp::network::tcp::server::ConnectionProvider::createShared(
                {"localhost", mPort, oatpp::network::Address::IP_4}));

        // Create Router component
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)
        (oatpp::web::server::HttpRouter::createShared());

        // Create ConnectionHandler component which uses Router component to route requests
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, serverConnectionHandler)([] {
            // get Router component
            OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
            auto httpConnectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);

            OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
            httpConnectionHandler->setErrorHandler(std::make_shared<ErrorHandler>(objectMapper));

            httpConnectionHandler->addRequestInterceptor(
                    std::make_shared<oatpp::web::server::interceptor::AllowOptionsGlobal>());
            httpConnectionHandler->addResponseInterceptor(
                    std::make_shared<oatpp::web::server::interceptor::AllowCorsGlobal>());
            httpConnectionHandler->addResponseInterceptor(std::make_shared<LoggerInterceptor>());

            return httpConnectionHandler;
        }());

#if TORREST_ENABLE_SWAGGER

        // General API docs
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>, swaggerDocumentInfo)
        (oatpp::swagger::DocumentInfo::Builder()
                 .setTitle("Torrest API")
                 .setDescription("C++ implementation of Torrest: a torrent streaming engine with a REST api")
                 .setVersion(TORREST_VERSION)
                 .setContactName("i96751414")
                 .setContactUrl("https://github.com/i96751414/torrest-cpp/")
                 .setLicenseName("MIT")
                 .setLicenseUrl("https://github.com/i96751414/torrest-cpp/blob/master/LICENSE")
                 .build());

#ifdef OATPP_SWAGGER_RES_PATH

        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>, swaggerResources)
        (oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH));

#endif //OATPP_SWAGGER_RES_PATH

#endif //TORREST_ENABLE_SWAGGER

    };

}}

#endif //TORREST_APP_COMPONENT_H
