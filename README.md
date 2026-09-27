# GigaCppConnect

**GigaCppConnect** is a modern C++20 framework for integrating and communicating with Sber's **GigaChat LLM** service.
It provides a type-safe interface, automatic OAuth 2.0 token management, token counting, account balance checks, multi-turn chat completions, and Kandinsky image generation and downloads.

## Implemented API Features

- [x] **OAuth 2.0 Authentication**: Automatic access token acquisition and refresh before expiration (`POST /oauth`)
- [x] **Models Catalog**: Retrieve and query available models (`GET /models`, `GET /models/{id}`)
- [x] **Chat Completions**: Send chat requests and receive streaming/non-streaming responses (`POST /chat/completions`)
- [x] **Token Counting**: Calculate tokens and character count for input texts (`POST /tokens/count`)
- [x] **Account Balance**: Retrieve remaining tokens and limits across models (`GET /balance`)
- [x] **File Management**: List files and retrieve metadata (`GET /files`, `GET /files/{id}`)
- [x] **Image Generation & Download**: Integrated Kandinsky generation, parsing, and binary download (`GenerateImage`, `DownloadImage`, `SaveImageToFile`)
- [x] **Embeddings**: Generate vector embeddings for text inputs (`POST /embeddings`)
- [x] **Flexible Credentials**: Initialize with `Client ID` + `Client Secret` or Base64 `Authorization Key`
- [x] **Cross-Platform Support**: Native Windows `WinHTTP` (zero external dependencies) and Linux / WSL (`libcurl`)
- [x] **Mock Backend**: Offline `MockClient` for isolated unit testing

## Requirements

- **C++20 compliant compiler**:
  - Windows: MSVC 2022 (v143+) / Clang 18+ / MinGW GCC 13+
  - Linux / WSL: GCC 11+ / Clang 14+ with `libcurl4-openssl-dev`
- **CMake**: 3.20 or newer

## Build and Run

### Windows (PowerShell / Visual Studio)

```powershell
# Configure
cmake -B build -S src

# Build
cmake --build build --config Release

# Run tests and demo
.\build\GigaCppConnect\Release\GigaCppConnect.exe
```

### Linux / WSL (Ubuntu / Debian)

```bash
# Install dependencies (one-time)
sudo apt update && sudo apt install -y build-essential cmake libcurl4-openssl-dev

# Configure
cmake -B build-linux -S src -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-linux -j$(nproc)

# Run tests and demo
./build-linux/GigaCppConnect/GigaCppConnect
```

## Usage Examples

### 1. Chat Dialog and Completions

```cpp
#include "GigaCppConnect.h"
#include <iostream>

using namespace GigaCpp;

int main() {
    // Initialize with Client ID and Client Secret
    GigaChatClient client(
        "029e38c6-b4db-4598-9209-febd65e551bb",
        "0db3909f-4624-4dc8-8ed8-2d8b70ff45d2",
        SCOPE_PERS,
        /*verifySsl=*/false
    );

    // Or initialize directly with Authorization Key:
    // GigaChatClient client("MDI5ZTM4YzYtYjRkYi00NTk4LTkyMDktZmViZDY1ZTU1MWJiOjBkYjM5MDlmLTQ2MjQtNGRjOC04ZWQ4LTJkOGI3MGZmNDVkMg==");

    // 1. Quick chat prompt:
    std::string reply = client.SimpleChat("Hello! Can you help me with C++20 concepts?");
    std::cout << "Reply:\n" << reply << "\n";

    // 2. Full request with system prompt and parameters:
    ChatRequest req;
    req.model = MODEL_GIGACHAT;
    req.messages = {
        {"system", "You are an expert modern C++ programmer."},
        {"user", "Explain C++20 concepts in two short sentences."}
    };
    req.temperature = 0.5f;

    ChatResponse resp = client.Chat(req);
    std::cout << resp.Content() << "\n";
    std::cout << "Tokens used: " << resp.usage.total_tokens << "\n";

    return 0;
}
```

### 2. Image Generation (Kandinsky) and Download

```cpp
#include "GigaCppConnect.h"
#include <iostream>

using namespace GigaCpp;

int main() {
    GigaChatClient client(
        "029e38c6-b4db-4598-9209-febd65e551bb",
        "0db3909f-4624-4dc8-8ed8-2d8b70ff45d2"
    );

    // Generate an image and save directly as a JPEG file:
    GigaImage img = client.GenerateImage("Draw a cute robot programmer in C++", MODEL_GIGACHAT, "robot.jpg");
    std::cout << "Saved image! File ID: " << img.file_id << " (" << img.size() << " bytes)\n";

    // Download an existing image by File ID:
    GigaImage downloaded = client.DownloadImage("b684c4fd-e596-4f48-a939-8bd9ee912921");
    client.SaveImageToFile(downloaded.file_id, "downloaded.jpg");

    return 0;
}
```

## Environment Variables

You can optionally configure credentials via environment variables:
- `GIGACHAT_CLIENT_ID`: Your client ID
- `GIGACHAT_CLIENT_SECRET`: Your client secret
- `GIGACHAT_AUTH_KEY`: Your Base64 authorization key

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
