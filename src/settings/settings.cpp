#include "settings.h"
#include <fstream>
#include <iomanip>

#include "utils/validation.h"

namespace torrest {
    // TODO: handle const nlohmann::json::exception &e
    Settings Settings::load(const std::string &path) {
        std::ifstream i(path);
        nlohmann::json j;
        i >> j;
        return j.get<Settings>();
    }

    Settings Settings::parse(const std::string &json) {
        nlohmann::json j = nlohmann::json::parse(json);
        return j.get<Settings>();
    }

    void Settings::save(const std::string &path) {
        std::ofstream o(path);
        nlohmann::json j = *this;
        o << std::setw(4) << j << std::endl;
    }

    std::string Settings::dump() {
        nlohmann::json j = *this;
        return j.dump(4);
    }

    void Settings::validate() const {
        VALIDATE(listen_port, GTE(0), LTE(65535))
        VALIDATE(alert_log_level, GTE(0), LT(spdlog::level::n_levels))
    }
}