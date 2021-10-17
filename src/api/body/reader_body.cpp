#include "reader_body.h"

namespace torrest { namespace api {

    ReaderBody::ReaderBody(std::shared_ptr<bittorrent::Reader> pReader, const v_int64 &pSize)
            : mReader(std::move(pReader)),
              mSize(pSize) {}

    oatpp::v_io_size ReaderBody::read(void *pBuffer, v_buff_size pCount, oatpp::async::Action &pAction) {
        return mReader->read(pBuffer, pCount);
    }

    void ReaderBody::declareHeaders(oatpp::web::protocol::http::Headers &pHeaders) {}

    p_char8 ReaderBody::getKnownData() {
        return nullptr;
    }

    v_int64 ReaderBody::getKnownSize() {
        return mSize;
    }

}}
