#include "empty_body.h"

namespace torrest { namespace api {

    EmptyBody::EmptyBody(const v_buff_size &pSize) : mSize(pSize) {}

    oatpp::v_io_size EmptyBody::read(void *pBuffer, v_buff_size pCount, oatpp::async::Action &pAction) {
        return 0;
    }

    void EmptyBody::declareHeaders(Headers &headers) {}

    p_char8 EmptyBody::getKnownData() {
        return nullptr;
    }

    v_buff_size EmptyBody::getKnownSize() {
        return mSize;
    }

}}
