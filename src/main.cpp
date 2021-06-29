#include <experimental/filesystem>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "oatpp/network/Server.hpp"
#include "oatpp/core/base/CommandLineArguments.hpp"
#include "oatpp-swagger/Controller.hpp"

#include "settings/settings.h"
#include "torrest.h"
#include "api/app_component.h"
#include "api/controller/settings.h"
#include "api/controller/service.h"
#include "api/controller/torrents.h"
#include "api/controller/files.h"
#include "utils/conversion.h"

#define ADD_CONTROLLER(name, type, router, doc) \
    auto name = std::make_shared<type>();       \
    name->addEndpointsToRouter(router);         \
    doc->pushBackAll(name->getEndpoints());

int main(int argc, const char *argv[]) {
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %l [%n] [thread-%t] %v");
    spdlog::set_level(spdlog::level::debug);
    auto logger = spdlog::stdout_logger_mt("main");

    auto cmdArgs = oatpp::base::CommandLineArguments(argc, argv);
    auto port = torrest::str_to_uint16(cmdArgs.getNamedArgumentValue("--port", "8080"));
    auto settingsPath = cmdArgs.getNamedArgumentValue("--settings", "settings.json");

    torrest::Settings settings;
    if (std::experimental::filesystem::exists(settingsPath)) {
        logger->debug("operation=main, message='Loading settings file', settingsPath='{}'", settingsPath);
        settings = torrest::Settings::load(settingsPath);
        settings.validate();
    } else {
        logger->debug("operation=main, message='Saving default settings file', settingsPath='{}'", settingsPath);
        settings.save(settingsPath);
    }

    logger->debug("operation=main, message='Initializing Torrest application'");
    torrest::Torrest::initialize(settingsPath, settings);

    logger->debug("operation=main, message='Starting OATPP environment'");
    oatpp::base::Environment::init();

    auto apiLogger = torrest::api::ApiLogger::get_instance();
    apiLogger->get_logger()->set_level(settings.api_log_level);
    oatpp::base::Environment::setLogger(apiLogger);

    {
        torrest::api::AppComponent component(port);
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);
        auto docEndpoints = oatpp::swagger::Controller::Endpoints::createShared();

        // Add all controllers
        ADD_CONTROLLER(settingsController, torrest::api::SettingsController, router, docEndpoints)
        ADD_CONTROLLER(serviceController, torrest::api::ServiceController, router, docEndpoints)
        ADD_CONTROLLER(torrentsController, torrest::api::TorrentsController, router, docEndpoints)
        ADD_CONTROLLER(filesController, torrest::api::FilesController, router, docEndpoints)

        // Add swagger
        auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints);
        swaggerController->addEndpointsToRouter(router);
        logger->debug("operation=main, message='Swagger available at http://localhost:{}/swagger/ui'", port);

        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
        oatpp::network::Server server(connectionProvider, connectionHandler);

        logger->info("operation=main, message='Starting HTTP server', port={}", port);
        server.run(static_cast<std::function<bool()>>([] {
            return torrest::Torrest::get_instance().is_running();
        }));

        logger->debug("operation=main, message='Destroying OATPP environment'");
        connectionProvider->stop();
        connectionHandler->stop();
    }

    logger->trace("operation=main, oatppObjectsCount={}", oatpp::base::Environment::getObjectsCount());
    logger->trace("operation=main, oatppObjectsCreated={}", oatpp::base::Environment::getObjectsCreated());
    oatpp::base::Environment::destroy();

    return 0;
}
