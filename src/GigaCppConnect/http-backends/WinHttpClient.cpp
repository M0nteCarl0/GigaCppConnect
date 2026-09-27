#include "WinHttpClient.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>

#include <iostream>
#include <vector>
#include <sstream>

#pragma comment(lib, "winhttp.lib")

namespace GigaCpp {

namespace {

std::wstring Utf8ToWide(std::string_view utf8Str) {
    if (utf8Str.empty()) return L"";
    int reqChars = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), nullptr, 0);
    if (reqChars <= 0) return L"";
    std::wstring wideStr(reqChars, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), static_cast<int>(utf8Str.size()), wideStr.data(), reqChars);
    return wideStr;
}

std::string WideToUtf8(std::wstring_view wideStr) {
    if (wideStr.empty()) return "";
    int reqBytes = WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), static_cast<int>(wideStr.size()), nullptr, 0, nullptr, nullptr);
    if (reqBytes <= 0) return "";
    std::string utf8Str(reqBytes, 0);
    WideCharToMultiByte(CP_UTF8, 0, wideStr.data(), static_cast<int>(wideStr.size()), utf8Str.data(), reqBytes, nullptr, nullptr);
    return utf8Str;
}

struct ParsedUrl {
    std::wstring host;
    INTERNET_PORT port{INTERNET_DEFAULT_HTTPS_PORT};
    std::wstring path;
    bool is_https{true};
    bool valid{false};
};

ParsedUrl ParseUrl(const std::string& urlStr) {
    ParsedUrl result;
    std::wstring wideUrl = Utf8ToWide(urlStr);

    URL_COMPONENTS urlComp{};
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwHostNameLength = static_cast<DWORD>(-1);
    urlComp.dwUrlPathLength = static_cast<DWORD>(-1);
    urlComp.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.length()), 0, &urlComp)) {
        return result;
    }

    result.host = std::wstring(urlComp.lpszHostName, urlComp.dwHostNameLength);
    result.port = urlComp.nPort;
    result.is_https = (urlComp.nScheme == INTERNET_SCHEME_HTTPS);

    std::wstring fullPath;
    if (urlComp.dwUrlPathLength > 0) {
        fullPath.append(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    } else {
        fullPath = L"/";
    }
    if (urlComp.dwExtraInfoLength > 0) {
        fullPath.append(urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
    }
    result.path = fullPath;
    result.valid = true;

    return result;
}

} // anonymous namespace

WinHttpClient::WinHttpClient() : WinHttpClient("GigaCppConnect/1.0 (Windows)") {}

WinHttpClient::WinHttpClient(const std::string& userAgent) : userAgent_(userAgent) {
    std::wstring wAgent = Utf8ToWide(userAgent_);
    hSession_ = WinHttpOpen(
        wAgent.c_str(),
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );
}

WinHttpClient::~WinHttpClient() {
    if (hSession_) {
        WinHttpCloseHandle(hSession_);
        hSession_ = nullptr;
    }
}

WinHttpClient::WinHttpClient(WinHttpClient&& other) noexcept
    : hSession_(other.hSession_), userAgent_(std::move(other.userAgent_)) {
    other.hSession_ = nullptr;
}

WinHttpClient& WinHttpClient::operator=(WinHttpClient&& other) noexcept {
    if (this != &other) {
        if (hSession_) {
            WinHttpCloseHandle(hSession_);
        }
        hSession_ = other.hSession_;
        userAgent_ = std::move(other.userAgent_);
        other.hSession_ = nullptr;
    }
    return *this;
}

HttpResponse WinHttpClient::Execute(const HttpRequest& request) {
    return Execute_impl(request);
}

HttpResponse WinHttpClient::Execute_impl(const HttpRequest& request) {
    HttpResponse response;

    if (!hSession_) {
        response.status_code = 0;
        response.error_message = "WinHttpOpen session handle is invalid";
        return response;
    }

    auto parsed = ParseUrl(request.url);
    if (!parsed.valid) {
        response.status_code = 0;
        response.error_message = "Failed to parse URL: " + request.url;
        return response;
    }

    HINTERNET hConnect = WinHttpConnect(hSession_, parsed.host.c_str(), parsed.port, 0);
    if (!hConnect) {
        response.status_code = 0;
        response.error_message = "WinHttpConnect failed with error: " + std::to_string(GetLastError());
        return response;
    }

    std::wstring wMethod = Utf8ToWide(request.method);
    DWORD openFlags = parsed.is_https ? WINHTTP_FLAG_SECURE : 0;

    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        wMethod.c_str(),
        parsed.path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        openFlags
    );

    if (!hRequest) {
        response.status_code = 0;
        response.error_message = "WinHttpOpenRequest failed with error: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hConnect);
        return response;
    }

    // Set timeouts
    int timeoutMs = request.timeout_seconds * 1000;
    WinHttpSetTimeouts(hRequest, timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    // SSL options
    DWORD secFlags = 0;
    if (!request.verify_ssl) {
        secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA |
                   SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
                   SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                   SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    // Headers
    if (!request.headers.empty()) {
        std::wstring headerStr;
        for (const auto& [key, val] : request.headers) {
            headerStr += Utf8ToWide(key) + L": " + Utf8ToWide(val) + L"\r\n";
        }
        WinHttpAddRequestHeaders(hRequest, headerStr.c_str(), static_cast<DWORD>(headerStr.length()),
                                 WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    // Send Request with retry on SSL / Client Cert
    BOOL sendSuccess = FALSE;
    for (int retry = 0; retry < 3; ++retry) {
        sendSuccess = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            const_cast<char*>(request.body.data()),
            static_cast<DWORD>(request.body.size()),
            static_cast<DWORD>(request.body.size()),
            0
        );

        if (sendSuccess) {
            break;
        }

        DWORD err = GetLastError();
        if (err == ERROR_WINHTTP_SECURE_FAILURE && !request.verify_ssl) {
            WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
            continue;
        }
        if (err == ERROR_WINHTTP_CLIENT_AUTH_CERT_NEEDED) {
            WinHttpSetOption(hRequest, WINHTTP_OPTION_CLIENT_CERT_CONTEXT, WINHTTP_NO_CLIENT_CERT_CONTEXT, 0);
            continue;
        }
        break;
    }

    if (!sendSuccess) {
        response.status_code = 0;
        response.error_message = "WinHttpSendRequest failed with error: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return response;
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        response.status_code = 0;
        response.error_message = "WinHttpReceiveResponse failed with error: " + std::to_string(GetLastError());
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        return response;
    }

    // Get Status Code
    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    if (WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX)) {
        response.status_code = static_cast<int>(statusCode);
    }

    // Read Response Body
    std::string responseBody;
    DWORD bytesRead = 0;
    do {
        DWORD bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable)) {
            break;
        }
        if (bytesAvailable == 0) {
            break;
        }

        std::vector<char> buffer(bytesAvailable);
        if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
            responseBody.append(buffer.data(), bytesRead);
        }
    } while (bytesRead > 0);

    response.body = std::move(responseBody);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);

    return response;
}

std::string WinHttpClient::Get_impl(const std::string& url) {
    HttpRequest req;
    req.url = url;
    req.method = "GET";
    return Execute_impl(req).body;
}

std::string WinHttpClient::Post_impl(const std::string& url, const std::string& body) {
    HttpRequest req;
    req.url = url;
    req.method = "POST";
    req.body = body;
    return Execute_impl(req).body;
}

} // namespace GigaCpp
