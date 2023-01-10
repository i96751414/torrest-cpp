#ifndef TORREST_VALIDATION_H
#define TORREST_VALIDATION_H

#include <string>

#define _VALIDATE1(var, v1) if (_GET_OPERATOR v1 (var _GET_VALIDATOR v1)) throw torrest::utils::ValidationException("'" #var "' " _GET_MESSAGE v1);
#define _VALIDATE2(var, v1, ...) _VALIDATE1(var, v1) _VALIDATE1(var, __VA_ARGS__)
#define _VALIDATE3(var, v1, ...) _VALIDATE1(var, v1) _VALIDATE2(var, __VA_ARGS__)
#define _GET_MACRO(_1, _2, _3, N, ...) N

#define _GET_OPERATOR(a, b, c) a
#define _GET_VALIDATOR(a, b, c) b
#define _GET_MESSAGE(a, b, c) c

#define LT(l) (!, < l, "must be less than " + std::to_string(l))
#define GT(l) (!, > l, "must be greater than " + std::to_string(l))
#define LTE(l) (!, <= l, "must be less than or equal to " + std::to_string(l))
#define GTE(l) (!, >= l, "must be greater than or equal to " + std::to_string(l))
#define NOT_EMPTY() (, .empty(), "cannot be empty")
#define VALIDATE(var, ...) do { _GET_MACRO(__VA_ARGS__, _VALIDATE3, _VALIDATE2, _VALIDATE1)(var, __VA_ARGS__) } while(0)

namespace torrest { namespace utils {

    class ValidationException : public std::runtime_error {
    public:
        explicit ValidationException(const char *pMessage) : std::runtime_error(pMessage) {}

        explicit ValidationException(const std::string &pMessage) : std::runtime_error(pMessage) {}
    };

}}

#endif //TORREST_VALIDATION_H
