cmake_minimum_required(VERSION 3.18.1)
project(mimi_mod)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -O3
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    -ffunction-sections
    -fdata-sections
)
add_link_options(-Wl,--gc-sections -Wl,-s)

set(ROOT_DIR       "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(SRC_DIR        "${ROOT_DIR}/src")
set(THIRD_PARTY    "${ROOT_DIR}/third_party")
set(IMGUI_DIR      "${THIRD_PARTY}/imgui")
set(JSON_DIR       "${THIRD_PARTY}/nlohmann")

if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(FATAL_ERROR "imgui not found at ${IMGUI_DIR}. Run scripts/setup_deps.bat first.")
endif()

include_directories(
    ${SRC_DIR}
    ${SRC_DIR}/include
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${JSON_DIR}
)

file(GLOB IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

file(GLOB_RECURSE UI_SOURCES       ${SRC_DIR}/ui/*.cpp)
file(GLOB_RECURSE CORE_SOURCES     ${SRC_DIR}/core/*.cpp)

add_library(subway_mod SHARED
    ${IMGUI_SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${SRC_DIR}/jni_entry.cpp
)

find_library(log-lib     log)
find_library(android-lib android)
find_library(egl-lib     EGL)
find_library(gles-lib    GLESv3)

target_link_libraries(subway_mod
    ${log-lib}
    ${android-lib}
    ${egl-lib}
    ${gles-lib}
    dl
)
