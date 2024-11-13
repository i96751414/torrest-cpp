#ifndef TORREST_MULTIPART_H
#define TORREST_MULTIPART_H

#include "oatpp/web/mime/multipart/Multipart.hpp"
#include "range_parser/range_parser.hpp"

#include "bittorrent/file.h"
#include "bittorrent/reader.h"

namespace torrest { namespace api {

    class RangeInputStream : public oatpp::data::stream::InputStream {
    public:
        static oatpp::data::stream::DefaultInitializedContext DEFAULT_CONTEXT;

        RangeInputStream(std::shared_ptr<bittorrent::Reader> pReader, v_buff_size pSize);

        void setInputStreamIOMode(oatpp::data::stream::IOMode ioMode) override ;

        oatpp::data::stream::IOMode getInputStreamIOMode() override;

        oatpp::data::stream::Context &getInputStreamContext() override;

        oatpp::v_io_size read(void *buffer, v_buff_size count, oatpp::async::Action &action) override;

    private:
        oatpp::data::stream::IOMode mIoMode;
        std::shared_ptr<bittorrent::Reader> mReader;
        v_buff_size mSize;
        v_buff_size mPosition;
    };

    class RangeResource : public oatpp::data::resource::Resource {
    public:
        RangeResource(std::shared_ptr<bittorrent::File> pFile, v_buff_size pSize, int64_t pOffset);

        std::shared_ptr<oatpp::data::stream::OutputStream> openOutputStream() override;

        std::shared_ptr<oatpp::data::stream::InputStream> openInputStream() override;

        oatpp::String getInMemoryData() override;

        v_int64 getKnownSize() override;

        oatpp::String getLocation() override;

    private:
        std::shared_ptr<bittorrent::File> mFile;
        v_buff_size mSize;
        int64_t mOffset;
    };

    class Multipart : public oatpp::web::mime::multipart::Multipart {
    public:
        typedef oatpp::web::mime::multipart::Part Part;

        explicit Multipart(std::shared_ptr<bittorrent::File> pFile, std::vector<range_parser::Range> pRanges, oatpp::String pMime);

        std::shared_ptr<Part> readNextPart(oatpp::async::Action &pAction) override;

        void writeNextPart(const std::shared_ptr<Part> &pPart, oatpp::async::Action &pAction) override;

    private:
        std::shared_ptr<bittorrent::File> mFile;
        std::vector<range_parser::Range> mRanges;
        oatpp::String mMime;
        int mNextRange;
    };

}}

#endif //TORREST_MULTIPART_H
