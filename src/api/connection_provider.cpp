#include "connection_provider.h"

#if defined(WIN32) || defined(_WIN32)
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

namespace torrest { namespace api {

    ConnectionProvider::ConnectionProvider(const oatpp::network::Address &pAddress,
                                           bool pUseExtendedConnections)
            : oatpp::network::tcp::server::ConnectionProvider(pAddress, pUseExtendedConnections),
              mInvalidator(std::make_shared<ConnectionInvalidator>()) {}

    oatpp::provider::ResourceHandle<oatpp::data::stream::IOStream> ConnectionProvider::get() {
        auto connection = oatpp::network::tcp::server::ConnectionProvider::get();
        connection.invalidator = mInvalidator;
        return connection;
    }

    void ConnectionProvider::ConnectionInvalidator::invalidate(
            const std::shared_ptr<oatpp::data::stream::IOStream> &pConnection) {
        auto c = std::static_pointer_cast<oatpp::network::tcp::Connection>(pConnection);
        oatpp::v_io_handle handle = c->getHandle();

#if defined(WIN32) || defined(_WIN32)
        shutdown(handle, SD_BOTH);
#else
        shutdown(handle, SHUT_RDWR);
#endif

        c->close();
    }

}}
