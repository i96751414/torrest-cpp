#ifndef TORREST_ERROR_RESPONSE_H
#define TORREST_ERROR_RESPONSE_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class ErrorResponse : public oatpp::DTO {

    DTO_INIT(ErrorResponse, DTO)

    FIELD(String, error, "Error message")

    static oatpp::data::mapping::type::DTOWrapper<ErrorResponse> create(const oatpp::String &pError) {
        auto response = ErrorResponse::createShared();
        response->error = pError;
        return response;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_ERROR_RESPONSE_H
