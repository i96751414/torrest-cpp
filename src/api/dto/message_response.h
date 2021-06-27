#ifndef TORREST_MESSAGE_RESPONSE_H
#define TORREST_MESSAGE_RESPONSE_H

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"
#include "oatpp/core/data/mapping/type/Object.hpp"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class MessageResponse : public oatpp::DTO {
    DTO_INIT(MessageResponse, DTO)

    DTO_FIELD_INFO(message) {
        info->description = "Response message";
    }

    DTO_FIELD(String, message);

    static oatpp::data::mapping::type::DTOWrapper<MessageResponse> create(const oatpp::String &pMessage) {
        auto response = MessageResponse::createShared();
        response->message = pMessage;
        return response;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_MESSAGE_RESPONSE_H
