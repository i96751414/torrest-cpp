#ifndef TORREST_MESSAGE_RESPONSE_H
#define TORREST_MESSAGE_RESPONSE_H

#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class MessageResponse : public oatpp::DTO {
    DTO_INIT(MessageResponse, DTO)

    FIELD(String, message, "Response message")

    static oatpp::data::mapping::type::DTOWrapper<MessageResponse> create(const oatpp::String &pMessage) {
        auto response = MessageResponse::createShared();
        response->message = pMessage;
        return response;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_MESSAGE_RESPONSE_H
