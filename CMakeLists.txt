cmake_minimum_required(VERSION 3.20)

project(SkyrimHeightmapEditor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.2b
)
FetchContent_MakeAvailable(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw)

if(WIN32)
    target_link_libraries(imgui PUBLIC opengl32)
endif()

add_executable(SkyrimHeightmapEditor
    src/main.cpp
    src/App.cpp
    src/App.h

    src/ui/MainWindow.cpp
    src/ui/MainWindow.h

    src/plugin/Plugin.cpp
    src/plugin/Plugin.h
    src/plugin/EspReader.cpp
    src/plugin/EspReader.h

    src/skyrim/Record.cpp
    src/skyrim/Record.h
    src/skyrim/SubRecord.cpp
    src/skyrim/SubRecord.h
    src/skyrim/SkyrimTypes.h

    src/terrain/HeightField.cpp
    src/terrain/HeightField.h
)

target_include_directories(SkyrimHeightmapEditor PRIVATE src)
target_link_libraries(SkyrimHeightmapEditor PRIVATE imgui)

if(MSVC)
    target_compile_options(SkyrimHeightmapEditor PRIVATE /W4 /permissive-)
else()
    target_compile_options(SkyrimHeightmapEditor PRIVATE -Wall -Wextra -Wpedantic)
endif()
