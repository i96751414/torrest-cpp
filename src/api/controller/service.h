#ifndef TORREST_SERVICE_CONTROLLER_H
#define TORREST_SERVICE_CONTROLLER_H

#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/web/mime/multipart/Reader.hpp"
#include "oatpp/web/mime/multipart/PartList.hpp"
#include "oatpp/web/mime/multipart/InMemoryPartReader.hpp"

#include "torrest.h"
#include "api/dto/message_response.h"
#include "api/dto/status_response.h"
#include "api/dto/new_torrent_response.h"
#include "api/dto/error_response.h"
#include "api/dto/torrent_multipart.h"
#include "utils/conversion.h"
#include "bittorrent/exceptions.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(ApiController)

class ServiceController : public oatpp::web::server::api::ApiController {
public:
    explicit ServiceController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper))
            : oatpp::web::server::api::ApiController(objectMapper) {}

    ENDPOINT_INFO(status) {
        info->summary = "Status";
        info->description = "Get the service status";
        info->addResponse<Object<StatusResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/status", status) {
        return createDtoResponse(Status::CODE_200, StatusResponse::create(
                Torrest::get_instance().get_service()->get_status()));
    }

    ENDPOINT_INFO(pause) {
        info->summary = "Pause";
        info->description = "Pause the service";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/pause", pause) {
        Torrest::get_instance().get_service()->pause();
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Service paused"));
    }

    ENDPOINT_INFO(resume) {
        info->summary = "Resume";
        info->description = "Resume the service";
        info->addResponse<Object<MessageResponse>>(Status::CODE_200, "application/json");
    }

    ENDPOINT("GET", "/resume", resume) {
        Torrest::get_instance().get_service()->resume();
        return createDtoResponse(Status::CODE_200, MessageResponse::create("Service resumed"));
    }

    ENDPOINT_INFO(addMagnet) {
        info->summary = "Add magnet";
        info->description = "Add magnet to the service";
        info->queryParams.add<String>("uri").description = "The magnet URI";
        info->queryParams.add<String>("uri").required = true;
        info->queryParams.add<Boolean>("download").description = "Start download after adding magnet";
        info->queryParams.add<Boolean>("download").required = false;
        info->queryParams.add<Boolean>("ignore_duplicate").description = "Ignore if duplicate";
        info->queryParams.add<Boolean>("ignore_duplicate").required = false;
        info->addResponse<Object<NewTorrentResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_400, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("GET", "/add/magnet", addMagnet,
             QUERY(String, uri, "uri"),
             QUERY(Boolean, download, "download", "false"),
             QUERY(Boolean, ignoreDuplicate, "ignore_duplicate", "false")) {

        auto magnet = unescape_string(uri->std_str());
        OATPP_ASSERT_HTTP(magnet.compare(0, 7, "magnet:") == 0, Status::CODE_400, "Invalid magnet provided")

        return handle_duplicate_torrent(
                [magnet, download] { return Torrest::get_instance().get_service()->add_magnet(magnet, download); },
                ignoreDuplicate);
    }

    ENDPOINT_INFO(addTorrent) {
        info->summary = "Add torrent file";
        info->description = "Add torrent file to the service";
        info->queryParams.add<Boolean>("download").description = "Start download after adding magnet";
        info->queryParams.add<Boolean>("download").required = false;
        info->queryParams.add<Boolean>("ignore_duplicate").description = "Ignore if duplicate";
        info->queryParams.add<Boolean>("ignore_duplicate").required = false;
        info->addConsumes<Object<TorrentMultipart>>("multipart/form-data");
        info->addResponse<Object<NewTorrentResponse>>(Status::CODE_200, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_400, "application/json");
        info->addResponse<Object<ErrorResponse>>(Status::CODE_500, "application/json");
    }

    ENDPOINT("POST", "/add/torrent", addTorrent,
             REQUEST(std::shared_ptr<IncomingRequest>, request),
             QUERY(Boolean, download, "download", "false"),
             QUERY(Boolean, ignoreDuplicate, "ignore_duplicate", "false")) {

        auto multipart = std::make_shared<oatpp::web::mime::multipart::PartList>(request->getHeaders());
        auto memoryReader = oatpp::web::mime::multipart::createInMemoryPartReader(20 * 1024 * 1024);
        oatpp::web::mime::multipart::Reader multipartReader(multipart.get());
        multipartReader.setPartReader("torrent", memoryReader);
        request->transferBody(&multipartReader);

        auto torrent = multipart->getNamedPart("torrent");
        OATPP_ASSERT_HTTP(torrent, Status::CODE_400, "torrent file needs to be provided")

        return handle_duplicate_torrent(
                [torrent, download] {
                    return Torrest::get_instance().get_service()->add_torrent_data(
                            torrent->getInMemoryData()->c_str(),
                            int(torrent->getInMemoryData()->getSize()), download);
                }, ignoreDuplicate);
    }

    std::shared_ptr<OutgoingResponse>
    handle_duplicate_torrent(const std::function<std::string(void)> &pFun, bool pIgnoreDuplicate) {
        std::string infoHash;

        if (pIgnoreDuplicate) {
            try {
                infoHash = pFun();
            } catch (const bittorrent::DuplicateTorrentException &e) {
                infoHash = e.get_info_hash();
            }
        } else {
            infoHash = pFun();
        }

        return createDtoResponse(Status::CODE_200, NewTorrentResponse::create(infoHash));
    }
};

#include OATPP_CODEGEN_END(ApiController)

}}

#endif //TORREST_SERVICE_CONTROLLER_H
