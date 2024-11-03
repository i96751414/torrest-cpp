#ifndef TORREST_ITEMS_H
#define TORREST_ITEMS_H

#include "api/dto/file_info_status.h"
#include "api/dto/folder_info_status.h"
#include "api/dto/utils.h"

namespace torrest { namespace api {

#include OATPP_CODEGEN_BEGIN(DTO)

class Items : public oatpp::DTO {
    DTO_INIT(Items, DTO)

    FIELD(List<Object<FolderInfoStatus>>, folders, "Folders")

    FIELD(List<Object<FileInfoStatus>>, files, "Files")

    static oatpp::data::mapping::type::DTOWrapper<Items> create(List<Object<FolderInfoStatus>> pFolderInfoList,
                                                                List<Object<FileInfoStatus>> pFileInfoList) {
        auto items = Items::createShared();
        items->folders = pFolderInfoList;
        items->files = pFileInfoList;
        return items;
    }
};

#include OATPP_CODEGEN_END(DTO)

}}

#endif //TORREST_ITEMS_H
