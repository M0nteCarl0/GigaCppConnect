# CMakeLists.txt: Top-level CMake project file.
cmake_minimum_required (VERSION 3.20)

# Enable Edit and Continue / Hot Reload for MSVC if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("GigaCppConnect" VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT option on MSVC for standalone zero-dependency distribution
option(GIGACPP_STATIC_CRT "Link MSVC C/C++ runtime statically (/MT)" ON)
if (MSVC AND GIGACPP_STATIC_CRT)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Static standard library on GCC/Clang (Linux)
option(GIGACPP_STATIC_STDLIB "Link libstdc++ and libgcc statically on Linux" ON)
if (UNIX AND NOT APPLE AND GIGACPP_STATIC_STDLIB)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")
endif()

# RPATH configuration for Linux so bundled dependencies in ../lib or ./lib are found automatically
if (UNIX AND NOT APPLE)
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib:$ORIGIN/lib:$ORIGIN")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

# Include subprojects
add_subdirectory ("GigaCppConnect")
