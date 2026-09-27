#ifndef GIGACPP_TYPES
#define GIGACPP_TYPES

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <optional>
#include <cstdint>
#include <chrono>

namespace GigaCpp {

// Scopes for OAuth
inline constexpr const char* SCOPE_PERS = "GIGACHAT_API_PERS";
inline constexpr const char* SCOPE_CORP = "GIGACHAT_API_CORP";
inline constexpr const char* SCOPE_B2B  = "GIGACHAT_API_B2B";

// Models
inline constexpr const char* MODEL_GIGACHAT       = "GigaChat";
inline constexpr const char* MODEL_GIGACHAT_PRO   = "GigaChat-Pro";
inline constexpr const char* MODEL_GIGACHAT_MAX   = "GigaChat-Max";
inline constexpr const char* MODEL_GIGACHAT_2     = "GigaChat-2";
inline constexpr const char* MODEL_GIGACHAT_2_PRO = "GigaChat-2-Pro";
inline constexpr const char* MODEL_GIGACHAT_2_MAX = "GigaChat-2-Max";
inline constexpr const char* MODEL_EMBEDDINGS     = "Embeddings";

// HTTP Request definition
struct HttpRequest {
    std::string url;
    std::string method{"GET"}; // GET, POST, DELETE, etc.
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
    bool verify_ssl{false};
    int timeout_seconds{30};
};

// HTTP Response definition
struct HttpResponse {
    int status_code{0};
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
    std::string error_message;

    [[nodiscard]] bool is_success() const noexcept {
        return status_code >= 200 && status_code < 300;
    }
};

// OAuth Token
struct AuthToken {
    std::string access_token;
    int64_t expires_at{0}; // milliseconds since epoch

    [[nodiscard]] bool IsExpired(int64_t leeway_seconds = 60) const noexcept {
        if (access_token.empty()) return true;
        auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        return (now + leeway_seconds * 1000) >= expires_at;
    }
};

// Model Information
struct ModelInfo {
    std::string id;
    std::string object;
    std::string owned_by;
    std::string type;
};

// Chat message
struct ChatMessage {
    std::string role; // "system", "user", "assistant", "function"
    std::string content;
};

// Chat completion request
struct ChatRequest {
    std::string model{"GigaChat"};
    std::vector<ChatMessage> messages;
    std::optional<float> temperature;
    std::optional<float> top_p;
    std::optional<int> n;
    std::optional<bool> stream;
    std::optional<int> max_tokens;
    std::optional<float> repetition_penalty;
    std::optional<int> update_interval;
    std::optional<std::string> function_call; // "auto", "none", etc.
};

// Chat choice
struct ChatChoice {
    ChatMessage message;
    int index{0};
    std::string finish_reason;
};

// Token usage
struct UsageInfo {
    int prompt_tokens{0};
    int completion_tokens{0};
    int total_tokens{0};
    int precached_prompt_tokens{0};
};

// Forward declaration of image extraction helper
inline std::vector<std::string> ExtractImageIdsFromText(const std::string& text);

// Chat completion response
struct ChatResponse {
    std::string id;
    std::string object;
    int64_t created{0};
    std::string model;
    std::vector<ChatChoice> choices;
    UsageInfo usage;

    [[nodiscard]] std::string Content() const {
        if (!choices.empty()) {
            return choices[0].message.content;
        }
        return "";
    }

    [[nodiscard]] std::vector<std::string> GetImageIds() const;
    [[nodiscard]] bool HasImages() const;
};

// Image representation
struct GigaImage {
    std::string file_id;
    std::vector<uint8_t> data;
    std::string filename;
    int64_t bytes{0};

    [[nodiscard]] bool empty() const noexcept { return data.empty(); }
    [[nodiscard]] size_t size() const noexcept { return data.size(); }
};

// Token count item
struct TokenCountItem {
    std::string object;
    int tokens{0};
    int characters{0};
};

// Balance item
struct BalanceItem {
    std::string usage;
    int64_t value{0};
};

// Embedding item
struct EmbeddingItem {
    std::string object;
    std::vector<float> embedding;
    int index{0};
};

// Embedding response
struct EmbeddingResponse {
    std::string object;
    std::vector<EmbeddingItem> data;
    std::string model;
};

// File info
struct FileInfo {
    std::string id;
    std::string object;
    int64_t bytes{0};
    int64_t created_at{0};
    std::string filename;
    std::string purpose;
    std::string access_policy;
};

// Helper implementation
inline std::vector<std::string> ExtractImageIdsFromText(const std::string& text) {
    std::vector<std::string> ids;
    // Format: <img src="file-uuid" .../>
    std::string tag = "<img";
    size_t pos = 0;
    while ((pos = text.find(tag, pos)) != std::string::npos) {
        size_t srcPos = text.find("src=", pos);
        if (srcPos == std::string::npos) break;
        srcPos += 4;
        if (srcPos < text.size() && (text[srcPos] == '"' || text[srcPos] == '\'')) {
            char quote = text[srcPos];
            size_t endQuote = text.find(quote, srcPos + 1);
            if (endQuote != std::string::npos) {
                ids.push_back(text.substr(srcPos + 1, endQuote - (srcPos + 1)));
            }
        }
        pos += 4;
    }
    return ids;
}

inline std::vector<std::string> ChatResponse::GetImageIds() const {
    return ExtractImageIdsFromText(Content());
}

inline bool ChatResponse::HasImages() const {
    return !GetImageIds().empty();
}

} // namespace GigaCpp

// Legacy types preserved for backward compatibility
class Response
{
public:
	int status{0};
	std::string message{""};
	std::string data{""};
};	

class Request
{
public:
    std::string message{""};
    std::string data{""};
};

#endif // GIGACPP_TYPES
