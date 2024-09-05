# CMakeList.txt: проект CMake для GigaCppConnect; включите исходный код и определения,
# укажите здесь логику для конкретного проекта.
#

set(CMAKE_CXX_STANDARD 20)
# Добавьте источник в исполняемый файл этого проекта.
add_executable (GigaCppConnect "GigaCppConnect.cpp" "GigaCppConnect.h")


if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GigaCppConnect PROPERTY CXX_STANDARD 20)
endif()

# TODO: Добавьте тесты и целевые объекты, если это необходимо.
