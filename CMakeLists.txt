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
# small. Skip the heavy modules (audio + network) we don't bind yet.
set(BUILD_SHARED_LIBS    ON  CACHE BOOL "" FORCE)
set(SFML_BUILD_AUDIO     OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_NETWORK   OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_EXAMPLES  OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_TEST_SUITE OFF CACHE BOOL "" FORCE)
set(SFML_BUILD_DOC       OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/SFML EXCLUDE_FROM_ALL)

# ---- the plugin shared lib -------------------------------------------------
set(PLUGIN_SOURCES
    src/PluginEntry.cpp
    src/WindowBindings.cpp
    src/GraphicsBindings.cpp
    src/PluginGlobals.hpp
    src/HandleRegistry.hpp
    src/BindingHelpers.hpp
)

add_library(mtype_sfml SHARED ${PLUGIN_SOURCES})

target_include_directories(mtype_sfml PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(mtype_sfml PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
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

# Windows: copy the SFML DLLs (and any transitive runtime deps SFML
# uses, e.g. FreeType, OpenAL — but we disabled audio so only the
# Window/Graphics/System SFML DLLs ship) alongside the plugin output.
if(WIN32)
    add_custom_command(TARGET mtype_sfml POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-graphics>
            $<TARGET_FILE:sfml-window>
            $<TARGET_FILE:sfml-system>
            $<TARGET_FILE_DIR:mtype_sfml>
        COMMENT "Copying SFML DLLs alongside mtype_sfml.dll"
    )
endif()
