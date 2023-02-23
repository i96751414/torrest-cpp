#include <iostream>

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "oatpp/network/Server.hpp"

#if TORREST_ENABLE_SWAGGER
#ifdef OATPP_SWAGGER_RES_PATH
#include "oatpp-swagger/Controller.hpp"
typedef oatpp::swagger::Controller SwaggerController;
#else

#include "api/controller/swagger.h"

typedef torrest::api::SwaggerController SwaggerController;
#endif
#endif

#include "torrest.h"
#include "api/app_component.h"
#include "api/controller/settings.h"
#include "api/controller/service.h"
#include "api/controller/torrents.h"
#include "api/controller/files.h"
#include "api/controller/serve.h"
#include "utils/conversion.h"

#if TORREST_LIBRARY
#ifdef _WIN32
#define EXPORT_C extern "C" __declspec(dllexport)
#else
#define EXPORT_C extern "C"
#endif
#else
#define EXPORT_C
#endif

struct Options {
    uint16_t port = 8080;
    std::string settings_path = "settings.json";
    spdlog::level::level_enum global_log_level = spdlog::level::info;
};

void start(const Options &options) {
    spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %l [%n] [thread-%t] %v");
    spdlog::set_level(options.global_log_level);
    auto logger = spdlog::stdout_logger_mt("main");

    logger->debug("operation=main, message='Initializing Torrest application', version=" TORREST_VERSION);
    torrest::Torrest::initialize(options.settings_path);

    logger->debug("operation=main, message='Starting OATPP environment'");
    oatpp::base::Environment::init();

    auto apiLogger = torrest::api::ApiLogger::get_instance();
    apiLogger->get_logger()->set_level(torrest::Torrest::get_instance().get_api_log_level());
    oatpp::base::Environment::setLogger(apiLogger);

    {
        torrest::api::AppComponent component(options.port);
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
        logger->debug("operation=main, message='Swagger available at http://localhost:{}/swagger/ui'", options.port);
#endif

        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
        oatpp::network::Server server(connectionProvider, connectionHandler);

        logger->info("operation=main, message='Starting HTTP server', port={}", options.port);
        server.run(static_cast<std::function<bool()>>([] {
            return torrest::Torrest::get_instance().is_running();
        }));

        logger->debug("operation=main, message='Destroying OATPP environment'");
        connectionProvider->stop();
        logger->trace("operation=main, message='Connection provider terminated'");
        connectionHandler->stop();
        logger->trace("operation=main, message='Connection handler terminated'");
    }

    logger->trace("operation=main, oatppObjectsCount={}", oatpp::base::Environment::getObjectsCount());
    logger->trace("operation=main, oatppObjectsCreated={}", oatpp::base::Environment::getObjectsCreated());
    oatpp::base::Environment::destroy();

    logger->trace("operation=main, message='Finished terminating'");
}

#if TORREST_LIBRARY

struct String {
    const char *ptr;
    size_t size;

    std::string to_string() const {
        return std::move(std::string(ptr, size));
    }
};

EXPORT_C int start(uint16_t port, String settings_path, int global_log_level) {
    int return_code = 0;
    Options options{
            .port=port,
            .settings_path=settings_path.to_string(),
            .global_log_level=static_cast<spdlog::level::level_enum>(global_log_level)};

    try {
        start(options);
    } catch (...) {
        return_code = 127;
    }

    torrest::Torrest::destroy();
    return return_code;
}

#else

namespace spdlog { namespace level {

    std::istream &operator>>(std::istream &pIs, level_enum &pLevel) {
        std::string levelString;
        pIs >> levelString;
        boost::algorithm::to_lower(levelString);

        pLevel = from_str(levelString);
        if (pLevel == off && levelString != "off") {
            pIs.setstate(std::ios_base::failbit);
        }

        return pIs;
    }

}}

Options parse_arguments(int argc, const char *argv[]) {
    Options options;
    boost::program_options::options_description optionsDescription("Optional arguments");
    optionsDescription.add_options()
            ("port,p", boost::program_options::value<uint16_t>(&options.port),
             "server listen port (default: 8080)")
            ("settings,s", boost::program_options::value<std::string>(&options.settings_path),
             "settings path (default: settings.json)")
            ("log-level", boost::program_options::value<spdlog::level::level_enum>(&options.global_log_level),
             "global log level (default: INFO)")
            ("version,v", "print version")
            ("help,h", "print help message");

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, optionsDescription), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << "Usage: " << boost::filesystem::path(argv[0]).filename().string() << " [options...]" << std::endl;
        std::cout << optionsDescription << std::endl;
        std::exit(0);
    } else if (vm.count("version")) {
        std::cout << TORREST_VERSION << std::endl;
        std::exit(0);
    }

    return options;
}

int main(int argc, const char *argv[]) {
    auto options = parse_arguments(argc, argv);
    start(options);
    return 0;
}

#endif //TORREST_LIBRARY
