#ifndef TORREST_DTO_UTILS_H
#define TORREST_DTO_UTILS_H

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#if TORREST_ENABLE_SWAGGER
#include "oatpp-swagger/Types.hpp"
typedef oatpp::swagger::Binary Binary;
#else
typedef oatpp::String Binary;
#endif //TORREST_ENABLE_SWAGGER

#define FIELD(type, name, desc) \
DTO_FIELD_INFO(name) {          \
    info->description = desc;   \
}                               \
DTO_FIELD(type, name);          \

#endif //TORREST_DTO_UTILS_H
