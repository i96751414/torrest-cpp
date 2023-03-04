#include "log.h"

#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"

namespace torrest { namespace utils {

    std::vector<spdlog::sink_ptr> &get_logger_sinks() {
        static std::vector<spdlog::sink_ptr> sinks {
#if !TORREST_LIBRARY
            std::make_shared<spdlog::sinks::stdout_sink_mt>()
#endif
        };
        return sinks;
    }

    void clear_sinks() {
        get_logger_sinks().clear();
    }

    void add_logger_sink(spdlog::sink_ptr pSink) {
        get_logger_sinks().push_back(std::move(pSink));
    }

    void add_stdout_sink() {
        add_logger_sink(std::make_shared<spdlog::sinks::stdout_sink_mt>());
    }

    void add_file_sink(const std::string &pFileName, bool pTruncate) {
        add_logger_sink(std::make_shared<spdlog::sinks::basic_file_sink_mt>(pFileName, pTruncate));
    }

    void add_callback_sink(const sinks::log_callback &pCallback) {
        add_logger_sink(std::make_shared<sinks::callback_sink_mt>(pCallback));
    }

    std::shared_ptr<spdlog::logger> create_logger(std::string pLoggerName) {
        spdlog::drop(pLoggerName);
        auto &sinks = get_logger_sinks();
        auto logger = std::make_shared<spdlog::logger>(std::move(pLoggerName), sinks.begin(), sinks.end());
        spdlog::initialize_logger(logger);
        return logger;
    }

}}
