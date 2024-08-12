#include <iostream>

#include "boost/program_options.hpp"
#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"
#include "spdlog/spdlog.h"
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

#include "api/app_component.h"
#include "api/controller/files.h"
#include "api/controller/serve.h"
#include "api/controller/service.h"
#include "api/controller/settings.h"
#include "api/controller/torrents.h"
#include "api/logger.h"
#include "torrest.h"
#include "utils/log.h"
#include "utils/utils.h"

#if TORREST_LIBRARY
#ifdef _WIN32
#define EXPORT_C extern "C" __declspec(dllexport)
#else
#define EXPORT_C extern "C"
#endif
#else
#define EXPORT_C
#endif

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

struct Options {
    uint16_t port = 8080;
    std::string settings_path = "settings.json";
    spdlog::level::level_enum global_log_level = spdlog::level::info;
    std::string log_pattern = "%Y-%m-%d %H:%M:%S.%e %l [%n] [thread-%t] %v";

    void parse_env(bool pValid = true) {
        torrest::utils::parse_env(port, "TORREST_PORT", pValid);
        torrest::utils::parse_env(settings_path, "TORREST_SETTINGS_PATH", pValid);
        torrest::utils::parse_env(global_log_level, "TORREST_GLOBAL_LOG_LEVEL", pValid);
        torrest::utils::parse_env(log_pattern, "TORREST_LOG_PATTERN", pValid);
    }
};

void start(const Options &options) {
    spdlog::set_pattern(options.log_pattern);
    spdlog::set_level(options.global_log_level);
    spdlog::set_automatic_registration(false);
    auto logger = torrest::utils::create_logger("main");

    logger->debug("operation=start, message='Initializing Torrest application', version=" TORREST_VERSION);
    torrest::Torrest::initialize(options.settings_path);

    logger->debug("operation=start, message='Starting OATPP environment'");
    auto apiLogger = std::make_shared<torrest::api::ApiLogger>(torrest::Torrest::get_instance()->get_api_logger());
    oatpp::base::Environment::init(apiLogger);

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
        logger->debug("operation=start, message='Swagger available at http://localhost:{}/swagger/ui'", options.port);
#endif

        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler);
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);
        oatpp::network::Server server(connectionProvider, connectionHandler);

        logger->info("operation=start, message='Starting HTTP server', port={}", options.port);
        server.run(static_cast<std::function<bool()>>([] {
            return torrest::Torrest::get_instance()->is_running();
        }));

        logger->debug("operation=start, message='Destroying OATPP environment'");
        connectionProvider->stop();
        logger->trace("operation=start, message='Connection provider terminated'");
        connectionHandler->stop();
        logger->trace("operation=start, message='Connection handler terminated'");
    }

    logger->trace("operation=start, oatppObjectsCount={}", oatpp::base::Environment::getObjectsCount());
    logger->trace("operation=start, oatppObjectsCreated={}", oatpp::base::Environment::getObjectsCreated());

    oatpp::base::Environment::destroy();
    torrest::Torrest::destroy();

    logger->trace("operation=start, message='Finished terminating'");
}

#if TORREST_LIBRARY

struct String {
    const char *ptr;
    size_t size;

    std::string to_string() const {
        return std::string(ptr, size);
    }
};

typedef void (*log_callback_fn)(int, String);

int start_with_options(const Options &options) {
    int return_code = 0;

    try {
        start(options);
    } catch (...) {
        oatpp::base::Environment::destroy();
        torrest::Torrest::destroy();
        return_code = 127;
    }

    return return_code;
}

EXPORT_C int start_with_env() {
    Options options;
    options.parse_env(false);
    return start_with_options(options);
}

EXPORT_C int start(uint16_t port, String settings_path, int global_log_level) {
    Options options{
            .port=port,
            .settings_path=settings_path.to_string(),
            .global_log_level=static_cast<spdlog::level::level_enum>(global_log_level)};

    return start_with_options(options);
}

EXPORT_C void stop() {
    torrest::Torrest::try_shutdown();
}

EXPORT_C void clear_logging_sinks() {
    torrest::utils::clear_sinks();
}

EXPORT_C void add_logging_stdout_sink() {
    torrest::utils::add_stdout_sink();
}

EXPORT_C void add_logging_file_sink(String file_path, bool truncate) {
    torrest::utils::add_file_sink(file_path.to_string(), truncate);
}

EXPORT_C void add_logging_callback_sink(log_callback_fn callback) {
    torrest::utils::add_callback_sink([callback](const spdlog::details::log_msg &pMsg) {
        String payload{.ptr=pMsg.payload.data(), .size=pMsg.payload.size()};
        callback(pMsg.level, payload);
    });
}

EXPORT_C const char* version() {
	return TORREST_VERSION;
}

#else

Options parse_arguments(int argc, const char *argv[]) {
    Options options;
    options.parse_env(false);

    std::string logPath;
    boost::program_options::options_description optionsDescription("Optional arguments");
    optionsDescription.add_options()
            ("port,p", boost::program_options::value<uint16_t>(&options.port),
             "server listen port (default: 8080)")
            ("settings,s", boost::program_options::value<std::string>(&options.settings_path),
             "settings path (default: settings.json)")
            ("log-level", boost::program_options::value<spdlog::level::level_enum>(&options.global_log_level),
             "global log level (default: INFO)")
            ("log-pattern", boost::program_options::value<std::string>(&options.log_pattern),
             "log pattern")
            ("log-path", boost::program_options::value<std::string>(&logPath),
             "log output path (default: not set)")
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

    if (!logPath.empty()) {
        torrest::utils::add_file_sink(logPath);
    }

    return options;
}

int main(int argc, const char *argv[]) {
    auto options = parse_arguments(argc, argv);
    start(options);
    return 0;
}

#endif //TORREST_LIBRARY
