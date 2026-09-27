#pragma once

#include "IHttpClient.h"
#include <string>
#include <vector>

namespace GigaCpp {

class WinHttpClient : public IHttpClient, public base_http_client<WinHttpClient> {
public:
    WinHttpClient();
    explicit WinHttpClient(const std::string& userAgent);
    ~WinHttpClient() override;

    WinHttpClient(const WinHttpClient&) = delete;
    WinHttpClient& operator=(const WinHttpClient&) = delete;
    WinHttpClient(WinHttpClient&& other) noexcept;
    WinHttpClient& operator=(WinHttpClient&& other) noexcept;

    HttpResponse Execute(const HttpRequest& request) override;
    HttpResponse Execute_impl(const HttpRequest& request);

    std::string Get_impl(const std::string& url);
    std::string Post_impl(const std::string& url, const std::string& body);

private:
    void* hSession_{nullptr}; // HINTERNET
    std::string userAgent_;
};

} // namespace GigaCpp
