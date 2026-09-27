#include "GigaChatClient.h"

#ifdef _WIN32
#include "WinHttpClient.h"
#endif

#if defined(__linux__) || defined(__APPLE__) || defined(GIGACPP_USE_CURL)
#include "CurlHttpClient.h"
#endif

#include "nlohmann/json.hpp"

#include <algorithm>
#include <iostream>
#include <fstream>

using json = nlohmann::json;

namespace GigaCpp {

namespace {

std::string Trim(std::string_view s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return std::string(s.substr(start, end - start + 1));
}

std::shared_ptr<IHttpClient> CreateDefaultHttpClient() {
#ifdef _WIN32
    return std::make_shared<WinHttpClient>();
#elif defined(__linux__) || defined(__APPLE__) || defined(GIGACPP_USE_CURL)
    return std::make_shared<CurlHttpClient>();
#else
    throw std::runtime_error("No default HTTP client available on this platform. Please pass a custom IHttpClient.");
#endif
}

} // anonymous namespace

GigaChatClient::GigaChatClient(
    std::string authKey,
    std::string scope,
    bool verifySsl,
    std::shared_ptr<IHttpClient> httpClient
) : authKey_(Trim(authKey)),
    scope_(std::move(scope)),
    verifySsl_(verifySsl),
    httpClient_(std::move(httpClient)) {
    if (!httpClient_) {
        httpClient_ = CreateDefaultHttpClient();
    }
}

GigaChatClient::GigaChatClient(
    const std::string& clientId,
    const std::string& clientSecret,
    std::string scope,
    bool verifySsl,
    std::shared_ptr<IHttpClient> httpClient
) : scope_(std::move(scope)),
    verifySsl_(verifySsl),
    httpClient_(std::move(httpClient)) {
    std::string combined = Trim(clientId) + ":" + Trim(clientSecret);
    authKey_ = Base64::Encode(combined);
    if (!httpClient_) {
        httpClient_ = CreateDefaultHttpClient();
    }
}

AuthToken GigaChatClient::Authenticate(bool forceRefresh) {
    std::lock_guard<std::mutex> lock(tokenMutex_);

    if (!forceRefresh && !currentToken_.IsExpired()) {
        return currentToken_;
    }

    HttpRequest req;
    req.url = oauthUrl_;
    req.method = "POST";
    req.verify_ssl = verifySsl_;
    req.headers = {
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "application/json"},
        {"RqUID", RqUUIDGenerator::GenerateRqUUID()},
        {"Authorization", "Basic " + authKey_}
    };
    req.body = "scope=" + scope_;

    HttpResponse resp = httpClient_->Execute(req);
    if (!resp.is_success()) {
        throw GigaChatException(
            "Authentication failed: HTTP " + std::to_string(resp.status_code) + 
            (resp.error_message.empty() ? "" : " (" + resp.error_message + ")") +
            " Body: " + resp.body,
            resp.status_code,
            resp.body
        );
    }

    try {
        json j = json::parse(resp.body);
        currentToken_.access_token = j.value("access_token", "");
        currentToken_.expires_at = j.value("expires_at", static_cast<int64_t>(0));
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse OAuth response JSON: ") + e.what() + " Raw: " + resp.body,
            resp.status_code,
            resp.body
        );
    }

    return currentToken_;
}

std::string GigaChatClient::GetAccessToken() {
    return Authenticate(false).access_token;
}

bool GigaChatClient::IsAuthenticated() {
    std::lock_guard<std::mutex> lock(tokenMutex_);
    return !currentToken_.IsExpired();
}

HttpResponse GigaChatClient::SendAuthorizedRequest(
    const std::string& method,
    const std::string& endpoint,
    const std::string& body,
    const std::string& contentType,
    const std::string& accept
) {
    std::string token = GetAccessToken();

    std::string url;
    if (endpoint.rfind("http://", 0) == 0 || endpoint.rfind("https://", 0) == 0) {
        url = endpoint;
    } else {
        url = apiBaseUrl_ + (endpoint.starts_with('/') ? endpoint : "/" + endpoint);
    }

    auto makeReq = [&](const std::string& tok) {
        HttpRequest req;
        req.url = url;
        req.method = method;
        req.verify_ssl = verifySsl_;
        req.headers = {
            {"Authorization", "Bearer " + tok},
            {"Accept", accept}
        };
        if (!body.empty()) {
            req.headers.emplace_back("Content-Type", contentType);
            req.body = body;
        }
        return req;
    };

    HttpResponse resp = httpClient_->Execute(makeReq(token));

    // If unauthorized, token may have been revoked or expired on server; refresh once
    if (resp.status_code == 401) {
        currentToken_ = Authenticate(true);
        resp = httpClient_->Execute(makeReq(currentToken_.access_token));
    }

    if (!resp.is_success()) {
        throw GigaChatException(
            "API request failed: " + method + " " + url + " returned HTTP " +
            std::to_string(resp.status_code) + " Body: " + resp.body,
            resp.status_code,
            resp.body
        );
    }

    return resp;
}

std::vector<ModelInfo> GigaChatClient::GetModels() {
    HttpResponse resp = SendAuthorizedRequest("GET", "/models");
    std::vector<ModelInfo> models;

    try {
        json j = json::parse(resp.body);
        if (j.contains("data") && j["data"].is_array()) {
            for (const auto& item : j["data"]) {
                ModelInfo info;
                info.id = item.value("id", "");
                info.object = item.value("object", "");
                info.owned_by = item.value("owned_by", "");
                info.type = item.value("type", "");
                models.push_back(std::move(info));
            }
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /models response JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return models;
}

ModelInfo GigaChatClient::GetModel(const std::string& modelId) {
    HttpResponse resp = SendAuthorizedRequest("GET", "/models/" + modelId);
    ModelInfo info;

    try {
        json j = json::parse(resp.body);
        info.id = j.value("id", "");
        info.object = j.value("object", "");
        info.owned_by = j.value("owned_by", "");
        info.type = j.value("type", "");
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /models/") + modelId + " JSON: " + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return info;
}

ChatResponse GigaChatClient::Chat(const ChatRequest& request) {
    json j;
    j["model"] = request.model;

    json messagesJson = json::array();
    for (const auto& msg : request.messages) {
        messagesJson.push_back({
            {"role", msg.role},
            {"content", msg.content}
        });
    }
    j["messages"] = messagesJson;

    if (request.temperature.has_value()) j["temperature"] = *request.temperature;
    if (request.top_p.has_value()) j["top_p"] = *request.top_p;
    if (request.n.has_value()) j["n"] = *request.n;
    if (request.stream.has_value()) j["stream"] = *request.stream;
    if (request.max_tokens.has_value()) j["max_tokens"] = *request.max_tokens;
    if (request.repetition_penalty.has_value()) j["repetition_penalty"] = *request.repetition_penalty;
    if (request.update_interval.has_value()) j["update_interval"] = *request.update_interval;
    if (request.function_call.has_value()) j["function_call"] = *request.function_call;

    std::string requestBody = j.dump();
    HttpResponse resp = SendAuthorizedRequest("POST", "/chat/completions", requestBody);

    ChatResponse chatResp;
    try {
        json respJson = json::parse(resp.body);
        chatResp.id = respJson.value("id", "");
        chatResp.object = respJson.value("object", "");
        chatResp.created = respJson.value("created", static_cast<int64_t>(0));
        chatResp.model = respJson.value("model", "");

        if (respJson.contains("choices") && respJson["choices"].is_array()) {
            for (const auto& choiceItem : respJson["choices"]) {
                ChatChoice choice;
                choice.index = choiceItem.value("index", 0);
                choice.finish_reason = choiceItem.value("finish_reason", "");
                if (choiceItem.contains("message")) {
                    choice.message.role = choiceItem["message"].value("role", "");
                    choice.message.content = choiceItem["message"].value("content", "");
                }
                chatResp.choices.push_back(std::move(choice));
            }
        }

        if (respJson.contains("usage") && respJson["usage"].is_object()) {
            chatResp.usage.prompt_tokens = respJson["usage"].value("prompt_tokens", 0);
            chatResp.usage.completion_tokens = respJson["usage"].value("completion_tokens", 0);
            chatResp.usage.total_tokens = respJson["usage"].value("total_tokens", 0);
            chatResp.usage.precached_prompt_tokens = respJson["usage"].value("precached_prompt_tokens", 0);
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /chat/completions JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return chatResp;
}

std::string GigaChatClient::SimpleChat(const std::string& prompt, const std::string& model) {
    ChatRequest req;
    req.model = model;
    req.messages.push_back({"user", prompt});
    ChatResponse resp = Chat(req);
    return resp.Content();
}

std::vector<TokenCountItem> GigaChatClient::CountTokens(
    const std::vector<std::string>& texts,
    const std::string& model
) {
    json j;
    j["model"] = model;
    j["input"] = texts;

    HttpResponse resp = SendAuthorizedRequest("POST", "/tokens/count", j.dump());
    std::vector<TokenCountItem> result;

    try {
        json respJson = json::parse(resp.body);
        if (respJson.is_array()) {
            for (const auto& item : respJson) {
                TokenCountItem tc;
                tc.object = item.value("object", "");
                tc.tokens = item.value("tokens", 0);
                tc.characters = item.value("characters", 0);
                result.push_back(std::move(tc));
            }
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /tokens/count JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return result;
}

std::vector<BalanceItem> GigaChatClient::GetBalance() {
    HttpResponse resp = SendAuthorizedRequest("GET", "/balance");
    std::vector<BalanceItem> balances;

    try {
        json j = json::parse(resp.body);
        if (j.contains("balance") && j["balance"].is_array()) {
            for (const auto& item : j["balance"]) {
                BalanceItem bi;
                bi.usage = item.value("usage", "");
                bi.value = item.value("value", static_cast<int64_t>(0));
                balances.push_back(std::move(bi));
            }
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /balance JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return balances;
}

EmbeddingResponse GigaChatClient::CreateEmbeddings(
    const std::vector<std::string>& texts,
    const std::string& model
) {
    json j;
    j["model"] = model;
    j["input"] = texts;

    HttpResponse resp = SendAuthorizedRequest("POST", "/embeddings", j.dump());
    EmbeddingResponse embeddingResp;

    try {
        json respJson = json::parse(resp.body);
        embeddingResp.object = respJson.value("object", "");
        embeddingResp.model = respJson.value("model", "");

        if (respJson.contains("data") && respJson["data"].is_array()) {
            for (const auto& item : respJson["data"]) {
                EmbeddingItem ei;
                ei.object = item.value("object", "");
                ei.index = item.value("index", 0);
                if (item.contains("embedding") && item["embedding"].is_array()) {
                    ei.embedding = item["embedding"].get<std::vector<float>>();
                }
                embeddingResp.data.push_back(std::move(ei));
            }
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /embeddings JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return embeddingResp;
}

std::vector<FileInfo> GigaChatClient::GetFiles() {
    HttpResponse resp = SendAuthorizedRequest("GET", "/files");
    std::vector<FileInfo> files;

    try {
        json j = json::parse(resp.body);
        if (j.contains("data") && j["data"].is_array()) {
            for (const auto& item : j["data"]) {
                FileInfo fi;
                fi.id = item.value("id", "");
                fi.object = item.value("object", "");
                fi.bytes = item.value("bytes", static_cast<int64_t>(0));
                fi.created_at = item.value("created_at", static_cast<int64_t>(0));
                fi.filename = item.value("filename", "");
                fi.purpose = item.value("purpose", "");
                files.push_back(std::move(fi));
            }
        }
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /files JSON: ") + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return files;
}

FileInfo GigaChatClient::GetFileInfo(const std::string& fileId) {
    HttpResponse resp = SendAuthorizedRequest("GET", "/files/" + fileId);
    FileInfo fi;

    try {
        json j = json::parse(resp.body);
        fi.id = j.value("id", "");
        fi.object = j.value("object", "");
        fi.bytes = j.value("bytes", static_cast<int64_t>(0));
        fi.created_at = j.value("created_at", static_cast<int64_t>(0));
        fi.filename = j.value("filename", "");
        fi.purpose = j.value("purpose", "");
        fi.access_policy = j.value("access_policy", "");
    } catch (const std::exception& e) {
        throw GigaChatException(
            std::string("Failed to parse /files/") + fileId + " JSON: " + e.what(),
            resp.status_code,
            resp.body
        );
    }

    return fi;
}

std::vector<uint8_t> GigaChatClient::DownloadFile(const std::string& fileId) {
    HttpResponse resp = SendAuthorizedRequest("GET", "/files/" + fileId + "/content", "", "application/json", "*/*");
    return std::vector<uint8_t>(resp.body.begin(), resp.body.end());
}

GigaImage GigaChatClient::DownloadImage(const std::string& fileId) {
    GigaImage img;
    img.file_id = fileId;

    try {
        FileInfo fi = GetFileInfo(fileId);
        img.filename = fi.filename.empty() ? (fileId + ".jpg") : fi.filename;
    } catch (...) {
        img.filename = fileId + ".jpg";
    }

    img.data = DownloadFile(fileId);
    img.bytes = static_cast<int64_t>(img.data.size());
    return img;
}

bool GigaChatClient::SaveImageToFile(const std::string& fileId, const std::string& filePath) {
    auto data = DownloadFile(fileId);
    if (data.empty()) return false;

    std::ofstream out(filePath, std::ios::binary);
    if (!out.is_open()) return false;

    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    return out.good();
}

std::vector<GigaImage> GigaChatClient::GenerateImages(
    const std::string& prompt,
    const std::string& model
) {
    ChatRequest req;
    req.model = model;
    req.function_call = "auto";
    req.messages = {{"user", prompt}};

    ChatResponse resp = Chat(req);
    auto imageIds = resp.GetImageIds();

    std::vector<GigaImage> images;
    for (const auto& id : imageIds) {
        images.push_back(DownloadImage(id));
    }

    return images;
}

GigaImage GigaChatClient::GenerateImage(
    const std::string& prompt,
    const std::string& model,
    const std::string& saveToFilePath
) {
    auto images = GenerateImages(prompt, model);
    if (images.empty()) {
        throw GigaChatException("No images were generated by the model for prompt: " + prompt);
    }

    if (!saveToFilePath.empty()) {
        std::ofstream out(saveToFilePath, std::ios::binary);
        if (!out.is_open()) {
            throw GigaChatException("Failed to open output file for writing: " + saveToFilePath);
        }
        out.write(reinterpret_cast<const char*>(images[0].data.data()), images[0].data.size());
    }

    return images[0];
}

} // namespace GigaCpp
