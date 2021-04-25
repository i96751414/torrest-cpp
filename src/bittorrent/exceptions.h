#ifndef TORREST_EXCEPTIONS_H
#define TORREST_EXCEPTIONS_H

namespace torrest {
    class BittorrentException : public std::runtime_error {
    public:
        explicit BittorrentException(const char *message) : std::runtime_error(message) {}
    };

    class InvalidInfoHashException : public BittorrentException {
    public:
        explicit InvalidInfoHashException(const char *message) : BittorrentException(message) {}
    };
}

#endif //TORREST_EXCEPTIONS_H
