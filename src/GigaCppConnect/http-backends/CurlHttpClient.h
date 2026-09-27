#pragma once

#include "IHttpClient.h"
#include <string>
#include <vector>

namespace GigaCpp {

class CurlHttpClient : public IHttpClient, public base_http_client<CurlHttpClient> {
public:
    CurlHttpClient();
    explicit CurlHttpClient(const std::string& userAgent);
    ~CurlHttpClient() override;

    CurlHttpClient(const CurlHttpClient&) = delete;
    CurlHttpClient& operator=(const CurlHttpClient&) = delete;
    CurlHttpClient(CurlHttpClient&& other) noexcept;
    CurlHttpClient& operator=(CurlHttpClient&& other) noexcept;

    HttpResponse Execute(const HttpRequest& request) override;
    HttpResponse Execute_impl(const HttpRequest& request);

    std::string Get_impl(const std::string& url);
    std::string Post_impl(const std::string& url, const std::string& body);

private:
    std::string userAgent_;
};

} // namespace GigaCpp
