# CMakeLists.txt: CMake project for GigaCppConnect
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    "GigaCppConnect.cpp"
    "core/GigaChatClient.cpp"
)

set(HEADERS
    "GigaCppConnect.h"
    "GigaCppTypes.h"
    "core/GigaChatClient.h"
    "core/RqUUIDGenerator.h"
    "core/Base64.h"
    "http-backends/IHttpClient.h"
    "http-backends/MockClient.h"
    "third_party/nlohmann/json.hpp"
)

if (WIN32)
    list(APPEND SOURCES "http-backends/WinHttpClient.cpp")
    list(APPEND HEADERS "http-backends/WinHttpClient.h")
else()
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    list(APPEND SOURCES "http-backends/CurlHttpClient.cpp")
    list(APPEND HEADERS "http-backends/CurlHttpClient.h")
endif()

add_executable(GigaCppConnect ${SOURCES} ${HEADERS})

target_include_directories(GigaCppConnect PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/http-backends
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)

if (MSVC)
    target_compile_options(GigaCppConnect PRIVATE /utf-8 /W4)
    target_compile_definitions(GigaCppConnect PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(GigaCppConnect PRIVATE -Wall -Wextra)
endif()

if (WIN32)
    target_link_libraries(GigaCppConnect PRIVATE winhttp)
else()
    target_link_libraries(GigaCppConnect PRIVATE CURL::libcurl Threads::Threads)
endif()
