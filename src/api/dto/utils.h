#ifndef TORREST_DTO_UTILS_H
#define TORREST_DTO_UTILS_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#define FIELD(type, name, desc) \
DTO_FIELD_INFO(name) {          \
    info->description = desc;   \
}                               \
DTO_FIELD(type, name);          \

#endif //TORREST_DTO_UTILS_H
