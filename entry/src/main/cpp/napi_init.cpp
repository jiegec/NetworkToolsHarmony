#include "napi/native_api.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <vector>
#include <string>
#include <assert.h>

static napi_value Add(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2] = {nullptr};

    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    napi_valuetype valuetype0;
    napi_typeof(env, args[0], &valuetype0);

    napi_valuetype valuetype1;
    napi_typeof(env, args[1], &valuetype1);

    double value0;
    napi_get_value_double(env, args[0], &value0);

    double value1;
    napi_get_value_double(env, args[1], &value1);

    napi_value sum;
    napi_create_double(env, value0 + value1, &sum);

    return sum;
}

// https://gist.github.com/jkomyno/45bee6e79451453c7bbdc22d033a282e

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen) {
    if (!sa) {
        strncpy(s, "Empty addr", maxlen);
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


static napi_value GetIfAddrs(napi_env env, napi_callback_info info) {
    struct ifaddrs *ifaddrs;
    int res = getifaddrs(&ifaddrs);
    assert(res == 0);

    // compute length
    struct ifaddrs *p = ifaddrs;
    int length = 0;
    while (p) {
        // skip af_packet
        if (p->ifa_addr->sa_family != AF_PACKET) {
            length++;
        }
        p = p->ifa_next;
    }

    napi_value arr;
    napi_create_array_with_length(env, length, &arr);

    p = ifaddrs;
    for (int i = 0; p; p = p->ifa_next) {
        // skip af_packet
        if (p->ifa_addr->sa_family == AF_PACKET) {
            continue;
        }

        // fill each entry with an object
        napi_value obj;
        napi_create_object(env, &obj);

        // name
        napi_value name_str;
        napi_create_string_utf8(env, p->ifa_name, NAPI_AUTO_LENGTH, &name_str);
        napi_set_named_property(env, obj, "name", name_str);

        // flags
        napi_value flags_int;
        napi_create_int32(env, p->ifa_flags, &flags_int);
        napi_set_named_property(env, obj, "flags", flags_int);

        // addr
        char addr_buffer[64];
        napi_value addr_str;
        get_ip_str(p->ifa_addr, addr_buffer, sizeof(addr_buffer));
        napi_create_string_utf8(env, addr_buffer, NAPI_AUTO_LENGTH, &addr_str);
        napi_set_named_property(env, obj, "addr", addr_str);

        // netmask
        char netmask_buffer[64];
        napi_value netmask_str;
        get_ip_str(p->ifa_netmask, netmask_buffer, sizeof(netmask_buffer));
        napi_create_string_utf8(env, netmask_buffer, NAPI_AUTO_LENGTH, &netmask_str);
        napi_set_named_property(env, obj, "netmask", netmask_str);

        // broadaddr
        char broadaddr_buffer[64];
        napi_value broadaddr_str;
        get_ip_str(p->ifa_broadaddr, broadaddr_buffer, sizeof(broadaddr_buffer));
        napi_create_string_utf8(env, broadaddr_buffer, NAPI_AUTO_LENGTH, &broadaddr_str);
        napi_set_named_property(env, obj, "broadaddr", broadaddr_str);

        // dstaddr
        char dstaddr_buffer[64];
        napi_value dstaddr_str;
        get_ip_str(p->ifa_dstaddr, dstaddr_buffer, sizeof(dstaddr_buffer));
        napi_create_string_utf8(env, dstaddr_buffer, NAPI_AUTO_LENGTH, &dstaddr_str);
        napi_set_named_property(env, obj, "dstaddr", dstaddr_str);

        napi_set_element(env, arr, i, obj);
        i++;
    }

    freeifaddrs(ifaddrs);
    return arr;
}


EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc[] = {
        {"add", nullptr, Add, nullptr, nullptr, nullptr, napi_default, nullptr},
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
