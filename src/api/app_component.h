#ifndef TORREST_APP_COMPONENT_H
#define TORREST_APP_COMPONENT_H

#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp-swagger/Model.hpp"
#include "oatpp-swagger/Resources.hpp"

#ifndef OATPP_SWAGGER_RES_PATH
#error oatpp-swagger/res is not defined (OATPP_SWAGGER_RES_PATH)
#endif

namespace torrest {
    /**
     *  Class which creates and holds Application components and registers components in oatpp::base::Environment.
     *  Order of components initialization is from top to bottom.
     */
    class AppComponent {
    public:
        explicit AppComponent(const uint16_t pPort) : mPort(pPort) {}

    private:
        uint16_t mPort;

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
            return oatpp::web::server::HttpConnectionHandler::createShared(router);
        }());

        // Create ObjectMapper component to serialize/deserialize DTOs in Controller's API
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, apiObjectMapper)
        (oatpp::parser::json::mapping::ObjectMapper::createShared());

        // General API docs
        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>, swaggerDocumentInfo)
        (oatpp::swagger::DocumentInfo::Builder()
                 .setTitle("Torrest API")
                 .setDescription("C++ implementation of Torrest: a torrent streaming engine with a REST api")
                 .setVersion("1.0")
                 .setContactName("i96751414")
                 .setContactUrl("https://github.com/i96751414/torrest-cpp/")
                 .setLicenseName("MIT")
                 .setLicenseUrl("https://github.com/i96751414/torrest-cpp/blob/master/LICENSE")
                 .build());

        OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>, swaggerResources)
        (oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH));
    };

}

#endif //TORREST_APP_COMPONENT_H
