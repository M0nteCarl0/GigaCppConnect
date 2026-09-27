#ifndef GIGACPPCONNECT
#define GIGACPPCONNECT

#define GIGACPPCONNECT_VERSION_MAJOR 0
#define GIGACPPCONNECT_VERSION_MINOR 2
#define GIGACPPCONNECT_VERSION_PATCH 0
#define GIGACPPCONNECT_VERSION ((GIGACPPCONNECT_VERSION_MAJOR << 16) | (GIGACPPCONNECT_VERSION_MINOR << 8) | GIGACPPCONNECT_VERSION_PATCH)

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>

#include "GigaCppTypes.h"
#include "core/RqUUIDGenerator.h"
#include "core/Base64.h"
#include "http-backends/IHttpClient.h"
#include "http-backends/MockClient.h"

#ifdef _WIN32
#include "http-backends/WinHttpClient.h"
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(GIGACPP_USE_CURL)
#include "http-backends/CurlHttpClient.h"
#endif

#include "core/GigaChatClient.h"

// Backward compatible helper functions
template <typename T>
std::string Post_test(GigaCpp::base_http_client<T>& client) {
    return client.Post("test", "test");
}

template <typename T>
std::string Get_test(GigaCpp::base_http_client<T>& client) {
    return client.Get("test");
}

#endif // GIGACPPCONNECT
