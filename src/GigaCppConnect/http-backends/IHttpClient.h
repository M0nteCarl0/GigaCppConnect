#pragma once

#include "GigaCppTypes.h"
#include <string>
#include <vector>
#include <memory>

namespace GigaCpp {

// Abstract HTTP client interface for polymorphism
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    virtual HttpResponse Execute(const HttpRequest& request) = 0;

    virtual HttpResponse Get(const std::string& url,
                             const std::vector<std::pair<std::string, std::string>>& headers = {},
                             bool verify_ssl = false) {
        HttpRequest req;
        req.url = url;
        req.method = "GET";
        req.headers = headers;
        req.verify_ssl = verify_ssl;
        return Execute(req);
    }

    virtual HttpResponse Post(const std::string& url,
                              const std::string& body,
                              const std::vector<std::pair<std::string, std::string>>& headers = {},
                              bool verify_ssl = false) {
        HttpRequest req;
        req.url = url;
        req.method = "POST";
        req.body = body;
        req.headers = headers;
        req.verify_ssl = verify_ssl;
        return Execute(req);
    }
};

// CRTP base class template preserving original design
template <typename Backend>
class base_http_client {
public:
    virtual ~base_http_client() = default;

    HttpResponse Execute(const HttpRequest& request) {
        return static_cast<Backend*>(this)->Execute_impl(request);
    }

    std::string Get(const std::string& url) {
        return static_cast<Backend*>(this)->Get_impl(url);
    }

    std::string Post(const std::string& url, const std::string& body) {
        return static_cast<Backend*>(this)->Post_impl(url, body);
    }
};

} // namespace GigaCpp

// Backwards-compatible alias in global namespace
template <typename Backend>
using base_http_client = GigaCpp::base_http_client<Backend>;
