#ifndef TORREST_EMPTY_BODY_H
#define TORREST_EMPTY_BODY_H

#include "oatpp/web/protocol/http/Http.hpp"
#include "oatpp/web/protocol/http/outgoing/Body.hpp"

namespace torrest { namespace api {

    class EmptyBody : public oatpp::web::protocol::http::outgoing::Body {
    public:
        explicit EmptyBody(const v_int64 &pSize);

        oatpp::v_io_size read(void *pBuffer, v_buff_size pCount, oatpp::async::Action &pAction) override;

        void declareHeaders(oatpp::web::protocol::http::Headers &pHeaders) override;

        p_char8 getKnownData() override;

        v_int64 getKnownSize() override;

    private:
        v_int64 mSize;
    };

}}

#endif //TORREST_EMPTY_BODY_H
