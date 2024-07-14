#include "ifaces.h"

#if defined(_WIN32) || defined(__USE_W32_SOCKETS)
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#else
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
 || defined(__bsdi__) || defined(__DragonFly__)
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include <arpa/inet.h>
#include <ifaddrs.h>

#endif

namespace torrest { namespace utils {

#if defined(_WIN32) || defined(__USE_W32_SOCKETS)

    using if_addrs = PIP_ADAPTER_ADDRESSES;
    using if_addrs_unicast = PIP_ADAPTER_UNICAST_ADDRESS;

    inline void free_ifs_addresses(if_addrs pAddresses) {
        HeapFree(GetProcessHeap(), 0, pAddresses);
    }

    inline int get_ifs_addresses(const int &pFamily, if_addrs *pAddresses) {
        ULONG bufLen = 15000;
        *pAddresses = (IP_ADAPTER_ADDRESSES *) HeapAlloc(GetProcessHeap(), 0, bufLen);
        if (*pAddresses == nullptr) return -1;
        DWORD res = GetAdaptersAddresses(pFamily, GAA_FLAG_INCLUDE_PREFIX, nullptr, *pAddresses, &bufLen);

        if (res == ERROR_BUFFER_OVERFLOW) {
            free_ifs_addresses(*pAddresses);
            *pAddresses = (IP_ADAPTER_ADDRESSES *) HeapAlloc(GetProcessHeap(), 0, bufLen);
            if (*pAddresses == nullptr) return -1;
            res = GetAdaptersAddresses(pFamily, GAA_FLAG_INCLUDE_PREFIX, nullptr, *pAddresses, &bufLen);
        }

        if (res != NO_ERROR) {
            free_ifs_addresses(*pAddresses);
            *pAddresses = nullptr;
            return -2;
        }

        return 0;
    }

    inline auto get_if_address(if_addrs_unicast pAddress) {
        return pAddress->Address.lpSockaddr;
    }

    inline auto get_if_address_length(if_addrs_unicast pAddress) {
        return pAddress->Address.iSockaddrLength;
    }

    inline auto get_next_address(if_addrs_unicast pAddress) {
        return pAddress->Next;
    }

#else

    using if_addrs = struct ifaddrs *;
    using if_addrs_unicast = struct ifaddrs *;

    inline void free_ifs_addresses(if_addrs pAddresses) {
        freeifaddrs(pAddresses);
    }

    inline int get_ifs_addresses(const int &pFamily, if_addrs *pAddresses) {
        (void) pFamily;
        return getifaddrs(pAddresses);
    }

    inline auto get_if_address(if_addrs_unicast pAddress) {
        return pAddress->ifa_addr;
    }

    inline auto get_if_address_length(if_addrs_unicast pAddress) {
        (void) pAddress;
        return sizeof(*pAddress->ifa_addr);
    }

    inline auto get_next_address(if_addrs_unicast pAddress) {
        return pAddress->ifa_next;
    }

#endif

    std::vector<ip_interface> list_interfaces_addresses(const int &pFamily) {
        std::vector<ip_interface> resultList;
        if_addrs adapterAddresses = nullptr;
        if (get_ifs_addresses(pFamily, &adapterAddresses) != 0) {
            return resultList;
        }

#if defined(_WIN32) || defined(__USE_W32_SOCKETS)
        for (auto adapter = adapterAddresses; adapter != nullptr; adapter = adapter->Next) {
            auto firstAddress = adapter->FirstUnicastAddress;
#else
        auto firstAddress = adapterAddresses;
#endif

        for (auto address = firstAddress; address != nullptr; address = get_next_address(address)) {
            auto ifAddress = get_if_address(address);

            if ((ifAddress == nullptr)
                || (ifAddress->sa_family != AF_INET && ifAddress->sa_family != AF_INET6)
                || ((pFamily == AF_INET || pFamily == AF_INET6) && pFamily != ifAddress->sa_family)) {
                continue;
            }

#if defined(_WIN32) || defined(__USE_W32_SOCKETS)
            auto interfaceName = adapter->AdapterName;
#else
            auto interfaceName = address->ifa_name;
#endif

#ifdef __MINGW32__
            char addr_str[NI_MAXHOST];
            if (getnameinfo(ifAddress, get_if_address_length(address),
                            addr_str, NI_MAXHOST, nullptr, 0, NI_NUMERICHOST) != 0) {
                continue;
            }
#else
            int address_family = ifAddress->sa_family;
            char addr_str[INET6_ADDRSTRLEN];
            void *src_addr;

            switch (address_family) {
                case AF_INET:
                    src_addr = &(reinterpret_cast<struct sockaddr_in *>(ifAddress)->sin_addr);
                    break;
                case AF_INET6:
                    src_addr = &(reinterpret_cast<struct sockaddr_in6 *>(ifAddress)->sin6_addr);
                    break;
                default:
                    continue;
            }

            inet_ntop(address_family, src_addr, addr_str, INET6_ADDRSTRLEN);
#endif

            resultList.push_back(ip_interface{
                    .name = std::string(interfaceName),
                    .address = std::string(addr_str),
                    .is_v4 = ifAddress->sa_family == AF_INET});
        }

#if defined(_WIN32) || defined(__USE_W32_SOCKETS)
        }
#endif

        if (adapterAddresses != nullptr) {
            free_ifs_addresses(adapterAddresses);
        }

        return resultList;
    }

    std::vector<ip_interface> list_interfaces_addresses_v4() {
        return list_interfaces_addresses(AF_INET);
    }

    std::vector<ip_interface> list_interfaces_addresses_v6() {
        return list_interfaces_addresses(AF_INET6);
    }

    std::vector<ip_interface> list_interfaces_addresses() {
        return list_interfaces_addresses(AF_UNSPEC);
    }

}}
