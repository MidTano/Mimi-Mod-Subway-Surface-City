cmake_minimum_required(VERSION 3.18)
project(mimi_mod_desktop CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ROOT_DIR    "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(SRC_DIR     "${ROOT_DIR}/src")
set(IMGUI_DIR   "${ROOT_DIR}/third_party/imgui")

if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(FATAL_ERROR "imgui not found at ${IMGUI_DIR}. Run scripts/setup_deps.bat first.")
endif()

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

file(GLOB_RECURSE UI_SOURCES ${SRC_DIR}/ui/*.cpp)

add_executable(mimi_mod_desktop
    main.cpp
    ${SRC_DIR}/core/runtime/runtime_state.cpp
    ${UI_SOURCES}
    ${IMGUI_SOURCES}
)

target_include_directories(mimi_mod_desktop PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${SRC_DIR}
    ${SRC_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mimi_mod_desktop
    opengl32
    gdi32
    user32
    dwmapi
    winmm
)
