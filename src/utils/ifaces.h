#ifndef TORREST_IFACES_H
#define TORREST_IFACES_H

#include <string>
#include <vector>

namespace torrest { namespace utils {

    struct ip_interface {
        std::string name;
        std::string address;
        bool is_v4;
    };

    std::vector<ip_interface> list_interfaces_addresses_v4();

    std::vector<ip_interface> list_interfaces_addresses_v6();

    std::vector<ip_interface> list_interfaces_addresses();

}}

#endif //TORREST_IFACES_H
