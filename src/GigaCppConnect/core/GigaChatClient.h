#pragma once

#include "GigaCppTypes.h"
#include "IHttpClient.h"
#include "RqUUIDGenerator.h"
#include "Base64.h"

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <stdexcept>

namespace GigaCpp {

class GigaChatException : public std::runtime_error {
public:
    GigaChatException(std::string message, int statusCode = 0, std::string responseBody = "")
        : std::runtime_error(message), statusCode_(statusCode), responseBody_(std::move(responseBody)) {}

    [[nodiscard]] int StatusCode() const noexcept { return statusCode_; }
    [[nodiscard]] const std::string& ResponseBody() const noexcept { return responseBody_; }

private:
    int statusCode_{0};
    std::string responseBody_;
};

class GigaChatClient {
public:
    // Construct using Authorization Key (Base64 ClientID:ClientSecret)
    explicit GigaChatClient(
        std::string authKey,
        std::string scope = SCOPE_PERS,
        bool verifySsl = false,
        std::shared_ptr<IHttpClient> httpClient = nullptr
    );

    // Construct using Client ID and Client Secret
    GigaChatClient(
        const std::string& clientId,
        const std::string& clientSecret,
        std::string scope = SCOPE_PERS,
        bool verifySsl = false,
        std::shared_ptr<IHttpClient> httpClient = nullptr
    );

    ~GigaChatClient() = default;

    // Authentication
    AuthToken Authenticate(bool forceRefresh = false);
    std::string GetAccessToken();
    bool IsAuthenticated();

    // Models API
    std::vector<ModelInfo> GetModels();
    ModelInfo GetModel(const std::string& modelId);

    // Chat Completions API
    ChatResponse Chat(const ChatRequest& request);
    std::string SimpleChat(const std::string& prompt, const std::string& model = MODEL_GIGACHAT);

    // Token Count API
    std::vector<TokenCountItem> CountTokens(
        const std::vector<std::string>& texts,
        const std::string& model = MODEL_GIGACHAT
    );

    // Balance API
    std::vector<BalanceItem> GetBalance();

    // Embeddings API
    EmbeddingResponse CreateEmbeddings(
        const std::vector<std::string>& texts,
        const std::string& model = MODEL_EMBEDDINGS
    );

    // Files and Images API
    std::vector<FileInfo> GetFiles();
    FileInfo GetFileInfo(const std::string& fileId);
    std::vector<uint8_t> DownloadFile(const std::string& fileId);
    GigaImage DownloadImage(const std::string& fileId);
    bool SaveImageToFile(const std::string& fileId, const std::string& filePath);

    // Image generation & retrieval
    GigaImage GenerateImage(
        const std::string& prompt,
        const std::string& model = MODEL_GIGACHAT,
        const std::string& saveToFilePath = ""
    );
    std::vector<GigaImage> GenerateImages(
        const std::string& prompt,
        const std::string& model = MODEL_GIGACHAT
    );

    // Configuration accessors
    void SetVerifySsl(bool verify) { verifySsl_ = verify; }
    [[nodiscard]] bool GetVerifySsl() const noexcept { return verifySsl_; }

    void SetScope(std::string scope) { scope_ = std::move(scope); }
    [[nodiscard]] const std::string& GetScope() const noexcept { return scope_; }

    void SetHttpClient(std::shared_ptr<IHttpClient> client) { httpClient_ = std::move(client); }
    [[nodiscard]] std::shared_ptr<IHttpClient> GetHttpClient() const { return httpClient_; }

    void SetOAuthUrl(std::string url) { oauthUrl_ = std::move(url); }
    [[nodiscard]] const std::string& GetOAuthUrl() const noexcept { return oauthUrl_; }

    void SetApiBaseUrl(std::string url) { apiBaseUrl_ = std::move(url); }
    [[nodiscard]] const std::string& GetApiBaseUrl() const noexcept { return apiBaseUrl_; }

private:
    std::string authKey_;
    std::string scope_;
    bool verifySsl_{false};

    std::string oauthUrl_{"https://ngw.devices.sberbank.ru:9443/api/v2/oauth"};
    std::string apiBaseUrl_{"https://gigachat.devices.sberbank.ru/api/v1"};

    std::shared_ptr<IHttpClient> httpClient_;
    AuthToken currentToken_;
    std::mutex tokenMutex_;

    HttpResponse SendAuthorizedRequest(
        const std::string& method,
        const std::string& endpoint,
        const std::string& body = "",
        const std::string& contentType = "application/json",
        const std::string& accept = "application/json"
    );
};

} // namespace GigaCpp
