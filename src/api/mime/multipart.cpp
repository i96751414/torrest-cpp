#include "multipart.h"

#include "oatpp/core/data/resource/InMemoryData.hpp"
#include "oatpp/web/protocol/http/Http.hpp"

namespace torrest { namespace api {

    oatpp::data::stream::DefaultInitializedContext RangeInputStream::DEFAULT_CONTEXT(oatpp::data::stream::StreamType::STREAM_FINITE);

    RangeInputStream::RangeInputStream(std::shared_ptr<bittorrent::Reader> pReader, v_buff_size pSize)
        : mIoMode(oatpp::data::stream::IOMode::ASYNCHRONOUS),
          mReader(std::move(pReader)),
          mSize(pSize),
          mPosition(0) {}

    void RangeInputStream::setInputStreamIOMode(oatpp::data::stream::IOMode ioMode) {
        mIoMode = ioMode;
    }

    oatpp::data::stream::IOMode RangeInputStream::getInputStreamIOMode() {
        return mIoMode;
    }

    oatpp::data::stream::Context &RangeInputStream::getInputStreamContext() {
        return DEFAULT_CONTEXT;
    }

    oatpp::v_io_size RangeInputStream::read(void *buffer, v_buff_size count, oatpp::async::Action &action) {
        v_buff_size readAmount = 0;
        v_buff_size desiredAmount = std::min(count, mSize - mPosition);
        if (desiredAmount > 0) {
            readAmount = mReader->read(buffer, desiredAmount);
            mPosition += readAmount;
        }
        return readAmount;
    }

    RangeResource::RangeResource(std::shared_ptr<bittorrent::File> pFile, v_buff_size pSize, int64_t pOffset)
        : mFile(std::move(pFile)),
          mSize(pSize),
          mOffset(pOffset) {}

    std::shared_ptr<oatpp::data::stream::OutputStream> RangeResource::openOutputStream() {
        throw std::runtime_error("No writes allowed");
    }

    std::shared_ptr<oatpp::data::stream::InputStream> RangeResource::openInputStream() {
        auto reader = mFile->reader();
        reader->seek(mOffset, std::ios::beg);
        return std::make_shared<RangeInputStream>(reader, mSize);
    }

    oatpp::String RangeResource::getInMemoryData() {
        return nullptr;
    }

    v_int64 RangeResource::getKnownSize() {
        return mSize;
    }

    oatpp::String RangeResource::getLocation() {
        return nullptr;
    }

    Multipart::Multipart(std::shared_ptr<bittorrent::File> pFile, std::vector<range_parser::Range> pRanges, oatpp::String pMime)
        : mFile(std::move(pFile)),
          mRanges(std::move(pRanges)),
          mMime(std::move(pMime)),
          mNextRange(0),
          oatpp::web::mime::multipart::Multipart(generateRandomBoundary()) {}

    std::shared_ptr<Multipart::Part> Multipart::readNextPart(oatpp::async::Action &pAction) {
        if (mNextRange >= mRanges.size()) {
            return {};
        }

        auto range = mRanges[mNextRange++];
        auto part = std::make_shared<Part>();
        part->putHeader(oatpp::web::protocol::http::Header::CONTENT_TYPE, mMime);
        part->putHeader(oatpp::web::protocol::http::Header::CONTENT_RANGE,
                        oatpp::String(range.content_range(mFile->get_size())));
        part->setPayload(std::make_shared<RangeResource>(mFile, range.length, range.start));
        return part;
    }

    void Multipart::writeNextPart(const std::shared_ptr<Part> &pPart, oatpp::async::Action &pAction) {
        throw std::runtime_error("No writes allowed");
    }

}}
