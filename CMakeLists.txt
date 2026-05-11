# mtype-sfml — runtime plugin exposing SFML 3 to mType.
#
# Builds mtype_sfml.{dll,so,dylib}, loaded by mType scripts via
# __plugin_load. Depends only on the C ABI declared in
# include/PluginHostApi.h (vendored — keep in sync with mType if
# MTYPE_PLUGIN_ABI_VERSION bumps) and vendor/SFML (3.x submodule).

cmake_minimum_required(VERSION 3.16)
project(mtype_sfml CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---- vendor: SFML 3 --------------------------------------------------------
# Build SFML as shared libs alongside the plugin so the plugin DLL stays
# small. Audio is enabled (SoundBuffer/Sound/Music/Listener bindings live
# in src/AudioBindings.cpp). Network is still disabled — no bindings for
# it yet.
set(BUILD_SHARED_LIBS    ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO     ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC       OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SFML EXCLUDE_FROM_ALL)

# ---- vendor: ImGui (docking branch) + imgui-sfml glue ----------------------
# Both are compiled directly into the plugin TU (no separate ImGui DLL). The
# imgui-sfml backend's imconfig-SFML.h is pulled in via IMGUI_USER_CONFIG so
# ImVec2 <-> sf::Vector2f conversions exist.
set(IMGUI_DIR        ${CMAKE_SOURCE_DIR}/vendor/imgui)
set(IMGUI_SFML_DIR   ${CMAKE_SOURCE_DIR}/vendor/imgui-sfml)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_SFML_DIR}/imgui-SFML.cpp
)

# ---- the plugin shared lib -------------------------------------------------
set(PLUGIN_SOURCES
    src/PluginEntry.cpp
    src/SystemBindings.cpp
    src/WindowBindings.cpp
    src/GraphicsBindings.cpp
    src/AudioBindings.cpp
    src/ImGuiBindings.cpp
    src/PluginGlobals.hpp
    src/HandleRegistry.hpp
    src/BindingHelpers.hpp
)

add_library(mtype_sfml SHARED ${PLUGIN_SOURCES} ${IMGUI_SOURCES})

target_include_directories(mtype_sfml PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_SFML_DIR}
)

# Tell imgui to swap its default imconfig.h for imgui-sfml's variant
# (the one that adds ImVec2 ↔ sf::Vector2f conversion operators).
target_compile_definitions(mtype_sfml PRIVATE
    IMGUI_USER_CONFIG="imconfig-SFML.h"
)

# OpenGL: imgui-sfml's renderer backend uses immediate-mode GL calls.
find_package(OpenGL REQUIRED)
target_link_libraries(mtype_sfml PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
    OpenGL::GL
)

set_target_properties(mtype_sfml PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mtype_sfml"
)

# Linux: visibility hidden by default — only the MTYPE_PLUGIN_EXPORT
# symbol leaks. Windows opts in via __declspec(dllexport).
if(UNIX AND NOT APPLE)
    set_target_properties(mtype_sfml PROPERTIES
        SUFFIX ".so"
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()

# Windows: copy the SFML DLLs alongside the plugin output. SFML 3's audio
# module statically links miniaudio, so no separate OpenAL DLL is needed.
if(WIN32)
    add_custom_command(TARGET mtype_sfml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE:sfml-audio>
            $<TARGET_FILE_DIR:mtype_sfml>
        COMMENT "Copying SFML DLLs alongside mtype_sfml.dll"
    )
endif()
