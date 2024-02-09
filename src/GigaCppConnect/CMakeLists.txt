# CMakeList.txt: проект CMake для GigaCppConnect; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

include(ExternalProject)
ExternalProject_Add(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG master
    PREFIX "${CMAKE_BINARY_DIR}/httplib"
    SOURCE_DIR "${CMAKE_BINARY_DIR}/httplib/src"
    BINARY_DIR "${CMAKE_BINARY_DIR}/httplib/build"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    UPDATE_COMMAND ""
)



# Добавьте источник в исполняемый файл этого проекта.
add_executable (GigaCppConnect "GigaCppConnect.cpp" "GigaCppConnect.h")

target_include_directories(GigaCppConnect PRIVATE "${CMAKE_BINARY_DIR}/httplib/src")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GigaCppConnect PROPERTY CXX_STANDARD 17)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
