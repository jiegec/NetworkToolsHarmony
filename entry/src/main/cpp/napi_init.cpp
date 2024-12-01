#include "napi/native_api.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <vector>
#include <string>
#include <assert.h>
#include <linux/if_link.h>
#include <linux/if.h>

// https://gist.github.com/jkomyno/45bee6e79451453c7bbdc22d033a282e

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
    if (!sa) {
        strncpy(s, "", maxlen);
        return NULL;
    }

    switch (sa->sa_family) {
    case AF_INET:
        inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), s, maxlen);
        break;

    case AF_INET6:
        inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
        break;

    default:
        snprintf(s, maxlen, "Unknown AF: %d", sa->sa_family);
        return NULL;
    }

    return s;
}

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
    char addr_buffer[64] = {0};
    napi_value addr_str;
    get_ip_str(value, addr_buffer, sizeof(addr_buffer));
    set_object_property_string(env, obj, key, addr_buffer);
}

static napi_value GetIfAddrs(napi_env env, napi_callback_info info) {
    struct ifaddrs *ifaddrs;
    int res = getifaddrs(&ifaddrs);
    assert(res == 0);

    // compute length
    struct ifaddrs *p = ifaddrs;
    int length = 0;
    while (p) {
        p = p->ifa_next;
    }

    napi_value arr;
    napi_create_array_with_length(env, length, &arr);

    p = ifaddrs;
    for (int i = 0; p; p = p->ifa_next) {
        // fill each entry with an object
        napi_value obj;
        napi_create_object(env, &obj);

        // name
        set_object_property_string(env, obj, "name", p->ifa_name);

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
            if (p->ifa_data) {
                struct rtnl_link_stats *stats = (struct rtnl_link_stats *)p->ifa_data;

                set_object_property_int32(env, obj, "tx_packets", stats->tx_packets);
                set_object_property_int32(env, obj, "tx_bytes", stats->tx_bytes);
                set_object_property_int32(env, obj, "rx_packets", stats->rx_packets);
                set_object_property_int32(env, obj, "rx_bytes", stats->rx_bytes);
            }
        } else {
            // addr
            set_object_property_sockaddr(env, obj, "addr", p->ifa_addr);

            // netmask
            set_object_property_sockaddr(env, obj, "netmask", p->ifa_netmask);

            // broadaddr
            set_object_property_sockaddr(env, obj, "broadaddr", p->ifa_broadaddr);

            // dstaddr
            set_object_property_sockaddr(env, obj, "dstaddr", p->ifa_dstaddr);
        }

        napi_set_element(env, arr, i, obj);
        i++;
    }

    freeifaddrs(ifaddrs);
    return arr;
}


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"getifaddrs", nullptr, GetIfAddrs, nullptr, nullptr, nullptr, napi_default, nullptr}};
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
