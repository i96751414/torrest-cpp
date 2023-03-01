#ifndef TORREST_LOG_H
#define TORREST_LOG_H

#include <vector>

#include "spdlog/spdlog.h"

namespace torrest { namespace utils {

    std::vector<spdlog::sink_ptr> &get_logger_sinks();

    void add_logger_sink(spdlog::sink_ptr pSink);

    void add_file_sink(const std::string &pFileName);

    std::shared_ptr<spdlog::logger> create_logger(std::string pLoggerName);

}}

#endif //TORREST_LOG_H
