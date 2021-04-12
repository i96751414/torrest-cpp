#ifndef TORREST_ERROR_RESPONSE_H
#define TORREST_ERROR_RESPONSE_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

namespace torrest {

#include OATPP_CODEGEN_BEGIN(DTO)

class ErrorResponse : public oatpp::DTO {

    DTO_INIT(ErrorResponse, DTO)

    DTO_FIELD_INFO(error) {
        info->description = "Error message";
    }

    DTO_FIELD(String, error);

};

#include OATPP_CODEGEN_END(DTO)

}

#endif //TORREST_ERROR_RESPONSE_H
