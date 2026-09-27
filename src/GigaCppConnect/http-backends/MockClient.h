#pragma once

#include "IHttpClient.h"
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <functional>

namespace GigaCpp {

class MockClient : public IHttpClient, public base_http_client<MockClient> {
public:
    std::string _url;
    std::string _body;
    std::string _result;

    std::unordered_map<std::string, HttpResponse> mock_responses;
    std::function<HttpResponse(const HttpRequest&)> custom_handler;

    MockClient() : _url("test"), _body("test") {}
    ~MockClient() override {
        _url.clear();
        _body.clear();
    }

    void SetMockResponse(const std::string& url, const HttpResponse& resp) {
        mock_responses[url] = resp;
    }

    HttpResponse Execute(const HttpRequest& request) override {
        return Execute_impl(request);
    }

    HttpResponse Execute_impl(const HttpRequest& request) {
        _url = request.url;
        _body = request.body;

        if (custom_handler) {
            return custom_handler(request);
        }

        auto it = mock_responses.find(request.url);
        if (it != mock_responses.end()) {
            return it->second;
        }

        HttpResponse resp;
        resp.status_code = 200;
        resp.body = "{\"mock\":\"ok\"}";
        return resp;
    }

    std::string Get_impl(const std::string& url) {
        _url = url;
        std::stringstream ss;
        ss << "GET Target url " << url << "\n";
        _result = ss.str();
        return _result;
    }

    std::string Post_impl(const std::string& url, const std::string& body) {
        _url = url;
        _body = body;
        std::stringstream ss;
        ss << "POST Target url " << url << "\n body: " << body;
        _result = ss.str();
        return _result;
    }
};

} // namespace GigaCpp

using MockClient = GigaCpp::MockClient;
