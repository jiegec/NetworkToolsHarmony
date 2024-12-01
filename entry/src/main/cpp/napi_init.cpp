#include "napi/native_api.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <vector>
#include <map>
#include <string>
#include <assert.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <linux/if.h>

// https://gist.github.com/jkomyno/45bee6e79451453c7bbdc22d033a282e

void set_object_property_string(napi_env env, napi_value obj, const char *key, const char *value) {
    napi_value str;
    napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &str);
    napi_set_named_property(env, obj, key, str);
}

void set_object_property_int32(napi_env env, napi_value obj, const char *key, int value) {
    napi_value num;
    napi_create_int32(env, value, &num);
    napi_set_named_property(env, obj, key, num);
}

void set_object_property_sockaddr(napi_env env, napi_value obj, const char *key, const struct sockaddr *value) {
    if (!value) {
        return;
    }

    char addr_buffer[64] = {0};
    char key_buffer[64] = {0};
    napi_value addr_str;

    switch (value->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET, &(((struct sockaddr_in *)value)->sin_addr), addr_buffer, sizeof(addr_buffer));
        snprintf(key_buffer, sizeof(key_buffer), "ipv4_%s", key);
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)value)->sin6_addr), addr_buffer, sizeof(addr_buffer));
        snprintf(key_buffer, sizeof(key_buffer), "ipv6_%s", key);
        break;

    default:
        return;
    }
    set_object_property_string(env, obj, key_buffer, addr_buffer);
}

// from: https://github.com/FRRouting/frr/blob/master/lib/prefix.c
uint8_t ip_masklen(struct in_addr netmask) {
    uint32_t tmp = ~ntohl(netmask.s_addr);

    /*
     * clz: count leading zeroes. sadly, the behaviour of this builtin is
     * undefined for a 0 argument, even though most CPUs give 32
     */
    return tmp ? __builtin_clz(tmp) : 32;
}

int ip6_masklen(struct in6_addr netmask) {
    if (netmask.s6_addr32[0] != 0xffffffffU)
        return __builtin_clz(~ntohl(netmask.s6_addr32[0]));
    if (netmask.s6_addr32[1] != 0xffffffffU)
        return __builtin_clz(~ntohl(netmask.s6_addr32[1])) + 32;
    if (netmask.s6_addr32[2] != 0xffffffffU)
        return __builtin_clz(~ntohl(netmask.s6_addr32[2])) + 64;
    if (netmask.s6_addr32[3] != 0xffffffffU)
        return __builtin_clz(~ntohl(netmask.s6_addr32[3])) + 96;
    /* note __builtin_clz(0) is undefined */
    return 128;
}

static napi_value GetIntfAddrs(napi_env env, napi_callback_info info) {
    struct ifaddrs *ifaddrs;
    int res = getifaddrs(&ifaddrs);
    assert(res == 0);

    // compute length
    std::vector<std::string> ifaces;
    struct ifaddrs *p = ifaddrs;
    while (p) {
        // collect iface names
        ifaces.push_back(p->ifa_name);
        p = p->ifa_next;
    }

    // sort and unique
    // https://stackoverflow.com/questions/1041620/whats-the-most-efficient-way-to-erase-duplicates-and-sort-a-vector
    std::sort(ifaces.begin(), ifaces.end());
    ifaces.erase(std::unique(ifaces.begin(), ifaces.end()), ifaces.end());

    napi_value arr;
    napi_create_array_with_length(env, ifaces.size(), &arr);

    // create a object for each interface
    std::map<std::string, napi_value> iface_map;
    for (int i = 0; i < ifaces.size(); i++) {
        napi_value obj;
        napi_create_object(env, &obj);

        // add to result arr
        napi_set_element(env, arr, i, obj);

        // name
        set_object_property_string(env, obj, "name", ifaces[i].c_str());
        iface_map[ifaces[i]] = obj;
    }

    // iterate again
    p = ifaddrs;
    for (; p; p = p->ifa_next) {
        // find corresponding object
        napi_value obj = iface_map[p->ifa_name];

        // flags
        std::string flags;
        std::vector<std::pair<int, std::string>> all_flags = {
            {IFF_UP, "UP"},
            {IFF_BROADCAST, "BROADCAST"},
            {IFF_DEBUG, "DEBUG"},
            {IFF_LOOPBACK, "LOOPBACK"},
            {IFF_POINTOPOINT, "POINTOPOINT"},
            {IFF_NOTRAILERS, "NOTRAILERS"},
            {IFF_RUNNING, "RUNNING"},
            {IFF_NOARP, "NOARP"},
            {IFF_PROMISC, "PROMISC"},
            {IFF_ALLMULTI, "ALLMULTI"},
            {IFF_MASTER, "MASTER"},
            {IFF_SLAVE, "SLAVE"},
            {IFF_MULTICAST, "MULTICAST"},
            {IFF_PORTSEL, "PORTSEL"},
            {IFF_AUTOMEDIA, "AUTOMEDIA"},
            {IFF_DYNAMIC, "DYNAMIC"},
            {IFF_LOWER_UP, "LOWER_UP"},
            {IFF_DORMANT, "DORMANT"},
            {IFF_ECHO, "ECHO"},
        };

        for (auto entry : all_flags) {
            if (p->ifa_flags & entry.first) {
                if (flags.size() != 0) {
                    flags += ",";
                }
                flags += entry.second;
            }
        }
        set_object_property_string(env, obj, "flags", flags.c_str());

        if (p->ifa_addr->sa_family == AF_PACKET) {
            // handle af_packet
            // parse mac address
            struct sockaddr_ll *addr = (struct sockaddr_ll *)p->ifa_addr;
            char mac_buffer[64];
            snprintf(mac_buffer, sizeof(mac_buffer), "%02x:%02x:%02x:%02x:%02x:%02x", addr->sll_addr[0],
                     addr->sll_addr[1], addr->sll_addr[2], addr->sll_addr[3], addr->sll_addr[4], addr->sll_addr[5]);
            set_object_property_string(env, obj, "mac", mac_buffer);

            // parse statistics
            if (p->ifa_data) {
                struct rtnl_link_stats *stats = (struct rtnl_link_stats *)p->ifa_data;

                set_object_property_int32(env, obj, "tx_packets", stats->tx_packets);
                set_object_property_int32(env, obj, "tx_bytes", stats->tx_bytes);
                set_object_property_int32(env, obj, "rx_packets", stats->rx_packets);
                set_object_property_int32(env, obj, "rx_bytes", stats->rx_bytes);
            }
        } else {
            // create addrs array if nonexistent
            bool result = false;
            napi_has_named_property(env, obj, "addrs", &result);
            napi_value addrs;
            if (!result) {
                napi_create_array(env, &addrs);
                napi_set_named_property(env, obj, "addrs", addrs);
            } else {
                napi_get_named_property(env, obj, "addrs", &addrs);
            }

            // add new addr
            napi_value new_addr;
            napi_create_object(env, &new_addr);

            // addr
            set_object_property_sockaddr(env, new_addr, "addr", p->ifa_addr);

            // netmask
            set_object_property_sockaddr(env, new_addr, "netmask", p->ifa_netmask);

            // broadaddr
            set_object_property_sockaddr(env, new_addr, "broadaddr", p->ifa_broadaddr);

            // dstaddr
            set_object_property_sockaddr(env, new_addr, "dstaddr", p->ifa_dstaddr);

            // compute prefix length
            int masklen = -1;
            if (p->ifa_addr->sa_family == AF_INET) {
                masklen = ip_masklen(((struct sockaddr_in *)p->ifa_netmask)->sin_addr);
            } else if (p->ifa_addr->sa_family == AF_INET6) {
                masklen = ip6_masklen(((struct sockaddr_in6 *)p->ifa_netmask)->sin6_addr);
            }
            set_object_property_int32(env, new_addr, "prefix", masklen);

            // append to array
            uint32_t len;
            napi_get_array_length(env, addrs, &len);
            napi_set_element(env, addrs, len, new_addr);
        }
    }

    freeifaddrs(ifaddrs);
    return arr;
}


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"get_intf_addrs", nullptr, GetIntfAddrs, nullptr, nullptr, nullptr, napi_default, nullptr}};
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}
EXTERN_C_END

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void) { napi_module_register(&demoModule); }
