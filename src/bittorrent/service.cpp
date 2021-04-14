#include "service.h"

#include "spdlog/sinks/stdout_sinks.h"

namespace torrest {

    Service::Service(const Settings &pSettings)
            : mLogger(spdlog::stdout_logger_mt("bittorrent")),
              mSettings(pSettings) {

    }
}