#include "boost/filesystem.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "oatpp/network/Server.hpp"
#include "oatpp/core/base/CommandLineArguments.hpp"

#if TORREST_ENABLE_SWAGGER
#ifdef OATPP_SWAGGER_RES_PATH
#include "oatpp-swagger/Controller.hpp"
typedef oatpp::swagger::Controller SwaggerController;
#else
#include "api/controller/swagger.h"
typedef torrest::api::SwaggerController SwaggerController;
#endif
#endif

#include "settings/settings.h"
#include "torrest.h"
#include "api/app_component.h"
#include "api/controller/settings.h"
#include "api/controller/service.h"
#include "api/controller/torrents.h"
#include "api/controller/files.h"
#include "api/controller/serve.h"
#include "utils/conversion.h"


int main(int argc, const char *argv[]) {
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %l [%n] [thread-%t] %v");
    spdlog::set_level(spdlog::level::debug);
    auto logger = spdlog::stdout_logger_mt("main");

    auto cmdArgs = oatpp::base::CommandLineArguments(argc, argv);
    auto port = torrest::utils::str_to_uint16(cmdArgs.getNamedArgumentValue("--port", "8080"));
    auto settingsPath = cmdArgs.getNamedArgumentValue("--settings", "settings.json");

    torrest::settings::Settings settings;
    if (boost::filesystem::exists(settingsPath)) {
        logger->debug("operation=main, message='Loading settings file', settingsPath='{}'", settingsPath);
        settings = torrest::settings::Settings::load(settingsPath);
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
#if TORREST_ENABLE_SWAGGER
        oatpp::web::server::api::Endpoints docEndpoints{};
#endif

        std::vector<std::shared_ptr<oatpp::web::server::api::ApiController>> controllers;
        controllers.emplace_back(std::make_shared<torrest::api::SettingsController>());
        controllers.emplace_back(std::make_shared<torrest::api::ServiceController>());
        controllers.emplace_back(std::make_shared<torrest::api::TorrentsController>());
        controllers.emplace_back(std::make_shared<torrest::api::FilesController>());
        controllers.emplace_back(std::make_shared<torrest::api::ServeController>());

        for (auto &controller : controllers) {
            router->addController(controller);
#if TORREST_ENABLE_SWAGGER
            docEndpoints.append(controller->getEndpoints());
#endif
        }

#if TORREST_ENABLE_SWAGGER
        router->addController(SwaggerController::createShared(docEndpoints));
        logger->debug("operation=main, message='Swagger available at http://localhost:{}/swagger/ui'", port);
#endif

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
