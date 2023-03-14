#ifndef TORREST_CONNECTION_PROVIDER_H
#define TORREST_CONNECTION_PROVIDER_H

#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

namespace torrest { namespace api {

    class ConnectionProvider : public oatpp::network::tcp::server::ConnectionProvider {
    public:
        explicit ConnectionProvider(const oatpp::network::Address &pAddress, bool pUseExtendedConnections);

        oatpp::provider::ResourceHandle<oatpp::data::stream::IOStream> get() override;

        static std::shared_ptr<ConnectionProvider>
        createShared(const oatpp::network::Address &pAddress, bool pUseExtendedConnections = false) {
            return std::make_shared<ConnectionProvider>(pAddress, pUseExtendedConnections);
        }

    private:
        class ConnectionInvalidator : public oatpp::provider::Invalidator<oatpp::data::stream::IOStream> {
        public:

            void invalidate(const std::shared_ptr<oatpp::data::stream::IOStream> &pConnection) override;

        };

        std::shared_ptr<ConnectionInvalidator> mInvalidator;
    };

}}

#endif
