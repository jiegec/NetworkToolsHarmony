#include "napi/native_api.h"
#include <sys/types.h>
#include <ifaddrs.h>
#include <vector>
#include <string>

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

static napi_value GetIfAddrs(napi_env env, napi_callback_info info) {
    struct ifaddrs *ifaddrs;
    int res = getifaddrs(&ifaddrs);
    std::vector<std::string> addrs;
    if (res == 0) {
        struct ifaddrs *p = ifaddrs;
        while (p) {
            addrs.push_back(p->ifa_name);
            p = p->ifa_next;
        }
        freeifaddrs(ifaddrs);
    }

    napi_value arr;
    napi_create_array_with_length(env, addrs.size(), &arr);

    for (size_t i = 0; i < addrs.size(); ++i) {
        napi_value str;
        napi_create_string_utf8(env, addrs[i].c_str(), NAPI_AUTO_LENGTH, &str);
        napi_set_element(env, arr, i, str);
    }

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
