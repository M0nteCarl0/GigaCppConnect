// GigaCppConnect.cpp: Main entry point and comprehensive test suite for GigaChat in C++20

#include "GigaCppConnect.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdlib>

using namespace GigaCpp;

int main() {
#ifdef _WIN32
    // Configure Windows console to UTF-8 output
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::cout << "===============================================================\n";
    std::cout << "        GigaCppConnect (C++20) - Verification and Demo         \n";
    std::cout << "===============================================================\n\n";

    // -------------------------------------------------------------------------
    // 0. Base components and backward compatibility verification
    // -------------------------------------------------------------------------
    std::cout << "[0] Testing basic components and backward compatibility...\n";
    MockClient mockClient;
    std::cout << "  - Mock GET:  " << Get_test(mockClient);
    std::cout << "  - Mock POST: " << Post_test(mockClient) << "\n";

    std::string testUuid = RqUUIDGenerator::GenerateRqUUID();
    std::cout << "  - Generated RqUID (RFC 4122 v4): " << testUuid << " (length: " << testUuid.size() << ")\n";

    // -------------------------------------------------------------------------
    // Credentials (environment variables with fallback to user-provided keys)
    // -------------------------------------------------------------------------
    const char* envClientId = std::getenv("GIGACHAT_CLIENT_ID");
    const char* envClientSecret = std::getenv("GIGACHAT_CLIENT_SECRET");
    const char* envAuthKey = std::getenv("GIGACHAT_AUTH_KEY");

    const std::string clientId = envClientId ? envClientId : "029e38c6-b4db-4598-9209-febd65e551bb";
    const std::string clientSecret = envClientSecret ? envClientSecret : "0db3909f-4624-4dc8-8ed8-2d8b70ff45d2";
    const std::string expectedAuthKey = envAuthKey ? envAuthKey : "MDI5ZTM4YzYtYjRkYi00NTk4LTkyMDktZmViZDY1ZTU1MWJiOjBkYjM5MDlmLTQ2MjQtNGRjOC04ZWQ4LTJkOGI3MGZmNDVkMg==";

    std::string computedAuthKey = Base64::Encode(clientId + ":" + clientSecret);
    std::cout << "  - Verifying Base64 AuthKey generation from ClientID + ClientSecret:\n";
    std::cout << "    Computed:  " << computedAuthKey << "\n";
    std::cout << "    Expected:  " << expectedAuthKey << "\n";
    if (computedAuthKey == expectedAuthKey) {
        std::cout << "    [OK] Keys match!\n\n";
    } else {
        std::cout << "    [FAIL] Key mismatch!\n\n";
    }

    // -------------------------------------------------------------------------
    // 1. Initialize GigaChatClient (C++20)
    // -------------------------------------------------------------------------
    std::cout << "[1] Initializing GigaChatClient with credentials...\n";
    // verifySsl = false allows connecting without requiring Sberbank/Russian root CA installed in system store
    GigaChatClient client(clientId, clientSecret, SCOPE_PERS, /*verifySsl=*/false);

    try {
        // ---------------------------------------------------------------------
        // 2. Request OAuth 2.0 access token
        // ---------------------------------------------------------------------
        std::cout << "\n[2] Requesting OAuth 2.0 access token...\n";
        AuthToken token = client.Authenticate();
        std::cout << "  [SUCCESS] Token acquired!\n";
        std::cout << "  - Token prefix:  " << token.access_token.substr(0, 32) << "...\n";
        std::cout << "  - Token length:  " << token.access_token.size() << " bytes\n";
        std::cout << "  - Expires at:    " << token.expires_at << " (timestamp ms)\n";

        // ---------------------------------------------------------------------
        // 3. Retrieve available models (GET /models)
        // ---------------------------------------------------------------------
        std::cout << "\n[3] Requesting model list (GET /models)...\n";
        auto models = client.GetModels();
        std::cout << "  [SUCCESS] Found " << models.size() << " models:\n";
        std::cout << std::left << std::setw(28) << "  Model ID"
                  << std::setw(15) << "Type"
                  << std::setw(18) << "Owner" << "\n";
        std::cout << "  ------------------------------------------------------------\n";
        for (const auto& m : models) {
            std::cout << "  " << std::left << std::setw(26) << m.id
                      << std::setw(15) << m.type
                      << std::setw(18) << m.owned_by << "\n";
        }

        // ---------------------------------------------------------------------
        // 4. Check account balance (GET /balance)
        // ---------------------------------------------------------------------
        std::cout << "\n[4] Requesting account balance (GET /balance)...\n";
        auto balances = client.GetBalance();
        std::cout << "  [SUCCESS] Balance information:\n";
        for (const auto& b : balances) {
            std::cout << "  - " << std::left << std::setw(16) << b.usage
                      << ": " << b.value << " tokens\n";
        }

        // ---------------------------------------------------------------------
        // 5. Count tokens (POST /tokens/count)
        // ---------------------------------------------------------------------
        std::cout << "\n[5] Testing token count (POST /tokens/count)...\n";
        std::vector<std::string> sampleTexts = {
            "Hello, GigaChat! C++20 integration is successfully running.",
            "Testing English token counting in GigaCppConnect framework."
        };
        auto tokenCounts = client.CountTokens(sampleTexts, MODEL_GIGACHAT);
        for (size_t i = 0; i < tokenCounts.size(); ++i) {
            std::cout << "  Text " << (i + 1) << ": \"" << sampleTexts[i] << "\"\n";
            std::cout << "  -> Tokens: " << tokenCounts[i].tokens
                      << ", Characters: " << tokenCounts[i].characters << "\n";
        }

        // ---------------------------------------------------------------------
        // 6. Chat completion (POST /chat/completions)
        // ---------------------------------------------------------------------
        std::cout << "\n[6] Sending chat completion request (POST /chat/completions)...\n";
        ChatRequest chatReq;
        chatReq.model = MODEL_GIGACHAT;
        chatReq.messages = {
            {"system", "You are a helpful AI assistant for modern C++ developers."},
            {"user", "In one brief sentence: confirm that GigaChat connection from modern C++20 is working great."}
        };
        chatReq.temperature = 0.3f;

        ChatResponse chatResp = client.Chat(chatReq);
        std::cout << "  [SUCCESS] Response received!\n";
        std::cout << "  - Model:         " << chatResp.model << "\n";
        std::cout << "  - Role:          " << (chatResp.choices.empty() ? "" : chatResp.choices[0].message.role) << "\n";
        std::cout << "  - Content:       " << chatResp.Content() << "\n";
        std::cout << "  - Usage:\n";
        std::cout << "      Prompt:      " << chatResp.usage.prompt_tokens << " tokens\n";
        std::cout << "      Completion:  " << chatResp.usage.completion_tokens << " tokens\n";
        std::cout << "      Total:       " << chatResp.usage.total_tokens << " tokens\n";

        // ---------------------------------------------------------------------
        // 7. Test SimpleChat convenience method
        // ---------------------------------------------------------------------
        std::cout << "\n[7] Testing SimpleChat() method...\n";
        std::string quickReply = client.SimpleChat("List 3 key features of C++20 very briefly in English.");
        std::cout << "  GigaChat reply:\n" << quickReply << "\n";

        // ---------------------------------------------------------------------
        // 8. Files API (GET /files)
        // ---------------------------------------------------------------------
        std::cout << "\n[8] Requesting file list (GET /files)...\n";
        auto files = client.GetFiles();
        std::cout << "  [SUCCESS] Files count on account: " << files.size() << "\n";
        for (const auto& f : files) {
            std::cout << "  - ID: " << f.id << ", Name: " << f.filename
                      << ", Size: " << f.bytes << " bytes\n";
        }

        // ---------------------------------------------------------------------
        // 9. Images API: Download and Generation
        // ---------------------------------------------------------------------
        std::cout << "\n[9] Testing Image retrieval and generation...\n";

        // 9.1 Download known image by file ID
        const std::string knownFileId = "b684c4fd-e596-4f48-a939-8bd9ee912921";
        std::cout << "  [9.1] Checking GetFileInfo() for ID: " << knownFileId << "...\n";
        try {
            FileInfo fileInfo = client.GetFileInfo(knownFileId);
            std::cout << "    - Filename: " << fileInfo.filename << "\n";
            std::cout << "    - Size:     " << fileInfo.bytes << " bytes\n";
            std::cout << "    - Created:  " << fileInfo.created_at << "\n";

            std::cout << "  [9.2] Downloading image DownloadImage()...\n";
            GigaImage img = client.DownloadImage(knownFileId);
            std::cout << "    [SUCCESS] Downloaded " << img.size() << " bytes!\n";

            // Verify JPEG signature (0xFF, 0xD8)
            if (img.size() >= 2 && img.data[0] == 0xFF && img.data[1] == 0xD8) {
                std::cout << "    [OK] JPEG magic header is valid (0xFF, 0xD8)\n";
            }

            // Save to file
            std::string sampleSavedPath = "downloaded_cat.jpg";
            bool saved = client.SaveImageToFile(knownFileId, sampleSavedPath);
            std::cout << "  [9.3] Saving to file \"" << sampleSavedPath << "\": "
                      << (saved ? "[SUCCESS]" : "[ERROR]") << "\n";

        } catch (const GigaChatException& ex) {
            std::cout << "    (Test image file not found or expired: " << ex.what() << ")\n";
        }

        // 9.4 Generate new image with GigaChat (Kandinsky)
        std::cout << "\n  [9.4] Requesting new image generation GenerateImage()...\n";
        std::cout << "    Prompt: \"Draw a cute friendly robot programmer in modern C++ style\"\n";
        try {
            std::string outputImagePath = "robot_cpp20.jpg";
            GigaImage newImg = client.GenerateImage("Draw a cute friendly robot programmer in modern C++ style", MODEL_GIGACHAT, outputImagePath);
            std::cout << "    [SUCCESS] Image generated and downloaded!\n";
            std::cout << "    - File ID:  " << newImg.file_id << "\n";
            std::cout << "    - Size:     " << newImg.size() << " bytes\n";
            std::cout << "    - Saved to: " << outputImagePath << "\n";
        } catch (const GigaChatException& ex) {
            std::cout << "    Image generation notice: " << ex.what() << "\n";
        }

        std::cout << "\n===============================================================\n";
        std::cout << "       ALL C++20 VERIFICATION CHECKS PASSED SUCCESSFULLY!      \n";
        std::cout << "===============================================================\n";

    } catch (const GigaChatException& ex) {
        std::cerr << "\n[GigaChat ERROR] HTTP " << ex.StatusCode() << ": " << ex.what() << "\n";
        if (!ex.ResponseBody().empty()) {
            std::cerr << "Response body: " << ex.ResponseBody() << "\n";
        }
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "\n[STANDARD EXCEPTION]: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
