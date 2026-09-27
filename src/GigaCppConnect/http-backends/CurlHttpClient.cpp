#include "CurlHttpClient.h"

#include <curl/curl.h>
#include <iostream>
#include <mutex>

namespace GigaCpp {

namespace {

class CurlGlobalInit {
public:
    CurlGlobalInit() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    ~CurlGlobalInit() {
        curl_global_cleanup();
    }
};

void EnsureCurlInitialized() {
    static CurlGlobalInit init;
}

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalBytes = size * nmemb;
    auto* responseBody = static_cast<std::string*>(userp);
    responseBody->append(static_cast<const char*>(contents), totalBytes);
    return totalBytes;
}

size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    size_t totalBytes = size * nitems;
    auto* headers = static_cast<std::vector<std::pair<std::string, std::string>>*>(userdata);
    std::string headerLine(buffer, totalBytes);

    auto colonPos = headerLine.find(':');
    if (colonPos != std::string::npos) {
        std::string key = headerLine.substr(0, colonPos);
        std::string val = headerLine.substr(colonPos + 1);

        // Trim CRLF/spaces
        auto trimLeft = val.find_first_not_of(" \t");
        if (trimLeft != std::string::npos) val = val.substr(trimLeft);
        auto trimRight = val.find_last_not_of(" \t\r\n");
        if (trimRight != std::string::npos) val = val.substr(0, trimRight + 1);

        headers->emplace_back(std::move(key), std::move(val));
    }
    return totalBytes;
}

} // anonymous namespace

CurlHttpClient::CurlHttpClient() : CurlHttpClient("GigaCppConnect/1.0 (Linux/cURL)") {}

CurlHttpClient::CurlHttpClient(const std::string& userAgent) : userAgent_(userAgent) {
    EnsureCurlInitialized();
}

CurlHttpClient::~CurlHttpClient() = default;

CurlHttpClient::CurlHttpClient(CurlHttpClient&& other) noexcept
    : userAgent_(std::move(other.userAgent_)) {}

CurlHttpClient& CurlHttpClient::operator=(CurlHttpClient&& other) noexcept {
    if (this != &other) {
        userAgent_ = std::move(other.userAgent_);
    }
    return *this;
}

HttpResponse CurlHttpClient::Execute(const HttpRequest& request) {
    return Execute_impl(request);
}

HttpResponse CurlHttpClient::Execute_impl(const HttpRequest& request) {
    HttpResponse response;

    CURL* curl = curl_easy_init();
    if (!curl) {
        response.status_code = 0;
        response.error_message = "Failed to initialize cURL handle";
        return response;
    }

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent_.c_str());

    // Method
    if (request.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
    } else if (request.method == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request.method.c_str());
        if (!request.body.empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
        }
    }

    // Headers
    struct curl_slist* headerList = nullptr;
    for (const auto& [key, val] : request.headers) {
        std::string headerStr = key + ": " + val;
        headerList = curl_slist_append(headerList, headerStr.c_str());
    }
    if (headerList) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
    }

    // SSL options
    if (!request.verify_ssl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    } else {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    }

    // Timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, static_cast<long>(request.timeout_seconds));

    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // Callbacks
    std::string responseBody;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    std::vector<std::pair<std::string, std::string>> responseHeaders;
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &responseHeaders);

    // Perform
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        response.status_code = 0;
        response.error_message = curl_easy_strerror(res);
    } else {
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        response.status_code = static_cast<int>(httpCode);
        response.body = std::move(responseBody);
        response.headers = std::move(responseHeaders);
    }

    if (headerList) {
        curl_slist_free_all(headerList);
    }
    curl_easy_cleanup(curl);

    return response;
}

std::string CurlHttpClient::Get_impl(const std::string& url) {
    HttpRequest req;
    req.url = url;
    req.method = "GET";
    return Execute_impl(req).body;
}

std::string CurlHttpClient::Post_impl(const std::string& url, const std::string& body) {
    HttpRequest req;
    req.url = url;
    req.method = "POST";
    req.body = body;
    return Execute_impl(req).body;
}

} // namespace GigaCpp
