#ifndef TORREST_READER_BODY_H
#define TORREST_READER_BODY_H

#include "oatpp/web/protocol/http/outgoing/Body.hpp"
#include "oatpp/web/protocol/http/Http.hpp"

#include "bittorrent/reader.h"

namespace torrest { namespace api {

    class ReaderBody : public oatpp::web::protocol::http::outgoing::Body {
    public:
        explicit ReaderBody(std::shared_ptr<bittorrent::Reader> pReader, const v_buff_size &pSize);

        oatpp::v_io_size read(void *pBuffer, v_buff_size pCount, oatpp::async::Action &pAction) override;

        void declareHeaders(oatpp::web::protocol::http::Headers &pHeaders) override;

        p_char8 getKnownData() override;

        v_buff_size getKnownSize() override;

    private:
        std::shared_ptr<bittorrent::Reader> mReader;
        v_buff_size mSize;
    };

}}

#endif //TORREST_READER_BODY_H
