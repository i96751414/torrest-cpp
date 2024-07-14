#ifndef TORREST_UTILS_H
#define TORREST_UTILS_H

#include <string>
#include <vector>

#include "boost/filesystem.hpp"
#include "boost/lexical_cast/try_lexical_convert.hpp"

namespace torrest { namespace utils {

    template<typename Target>
    void parse_env(Target &pTarget, const char *pEnv, bool pValid = true) {
        auto env_value = std::getenv(pEnv);
        if (env_value != nullptr && !boost::conversion::detail::try_lexical_convert(env_value, pTarget) && pValid) {
            std::ostringstream desc;
            desc << "Unable to parse " << pEnv << " environment variable";
            throw std::runtime_error(desc.str());
        }
    }

    template<typename... Args>
    boost::filesystem::path join_path(const boost::filesystem::path &pBasePath, Args const &...pArgs) {
        boost::filesystem::path path(pBasePath);
        boost::filesystem::path arg;
        using unpack = int[];
        (void) unpack{0, (path = (arg = pArgs).is_absolute() ? arg : path / arg, 0)...};
        return path;
    }

    std::string sanitize_ip_address(const std::string &pIp, const int &pLeadingGroups = 2);

    std::string join_string_vector(const std::vector<std::string> &pVector, const std::string &pDelimiter);

    std::string unescape_string(const std::string &pStr);

    inline std::string &ltrim(std::string &pStr, const char *pChars = " \t\n\r\f\v") {
        pStr.erase(0, pStr.find_first_not_of(pChars));
        return pStr;
    }

    inline std::string ltrim_copy(std::string pStr, const char *pChars = " \t\n\r\f\v") {
        return ltrim(pStr, pChars);
    }

    inline bool starts_with(const std::string &pStr, const std::string &pStart) {
        return pStr.rfind(pStart, 0) == 0;
    }

}}

#endif //TORREST_UTILS_H
