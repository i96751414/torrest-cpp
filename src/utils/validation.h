#ifndef TORREST_VALIDATION_H
#define TORREST_VALIDATION_H

#define _VALIDATE1(var, v1) if (!(var _GET_VALIDATOR v1)) throw torrest::validation_exception("'" #var "' " _GET_MESSAGE v1);
#define _VALIDATE2(var, v1, ...) _VALIDATE1(var, v1) _VALIDATE1(var, __VA_ARGS__)
#define _VALIDATE3(var, v1, ...) _VALIDATE1(var, v1) _VALIDATE2(var, __VA_ARGS__)
#define _GET_MACRO(_1, _2, _3, N, ...) N

#define _GET_VALIDATOR(a, b) a
#define _GET_MESSAGE(a, b) b

#define LT(l) (< l, "must be less than " # l)
#define LTE(l) (<= l, "must be less than or equal to " # l)
#define GTE(l) (>= l, "must be greater than or equal to " # l)
#define VALIDATE(var, ...) _GET_MACRO(__VA_ARGS__, _VALIDATE3, _VALIDATE2, _VALIDATE1)(var, __VA_ARGS__)

namespace torrest {
    class validation_exception : public std::runtime_error {
    public:
        explicit validation_exception(const char *message) : std::runtime_error(message) {}
    };
}

#endif //TORREST_VALIDATION_H
