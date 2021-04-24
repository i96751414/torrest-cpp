#ifndef TORREST_EXCEPTIONS_H
#define TORREST_EXCEPTIONS_H

namespace torrest {
    class InvalidInfoHashException : public std::runtime_error {
    public:
        explicit InvalidInfoHashException(const char *message) : std::runtime_error(message) {}
    };
}

#endif //TORREST_EXCEPTIONS_H
