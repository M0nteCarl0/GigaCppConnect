# CMakeLists.txt: CMake project for GigaCppConnect
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SOURCES
    "core/GigaChatClient.cpp"
)

set(LIB_HEADERS
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
    list(APPEND LIB_SOURCES "http-backends/WinHttpClient.cpp")
    list(APPEND LIB_HEADERS "http-backends/WinHttpClient.h")
else()
    find_package(CURL REQUIRED)
    find_package(Threads REQUIRED)
    list(APPEND LIB_SOURCES "http-backends/CurlHttpClient.cpp")
    list(APPEND LIB_HEADERS "http-backends/CurlHttpClient.h")
endif()

# Core static library
add_library(gigachat_client STATIC ${LIB_SOURCES} ${LIB_HEADERS})

target_include_directories(gigachat_client PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/http-backends>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/core>
    $<INSTALL_INTERFACE:include/http-backends>
    $<INSTALL_INTERFACE:include/third_party>
)

if (MSVC)
    target_compile_options(gigachat_client PRIVATE /utf-8 /W4)
    target_compile_definitions(gigachat_client PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(gigachat_client PRIVATE -Wall -Wextra)
endif()

if (WIN32)
    target_link_libraries(gigachat_client PUBLIC winhttp)
else()
    target_link_libraries(gigachat_client PUBLIC CURL::libcurl Threads::Threads)
endif()

# Verification / demo executable
add_executable(GigaCppConnect "GigaCppConnect.cpp")
target_link_libraries(GigaCppConnect PRIVATE gigachat_client)

if (MSVC)
    target_compile_options(GigaCppConnect PRIVATE /utf-8 /W4)
    target_compile_definitions(GigaCppConnect PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(GigaCppConnect PRIVATE -Wall -Wextra)
endif()

# Installation rules
include(GNUInstallDirs)

install(TARGETS GigaCppConnect gigachat_client
    EXPORT GigaCppConnectTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES
    "GigaCppConnect.h"
    "GigaCppTypes.h"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY "core/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/core
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "http-backends/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/http-backends
    FILES_MATCHING PATTERN "*.h"
)

install(DIRECTORY "third_party/"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/third_party
    FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)
