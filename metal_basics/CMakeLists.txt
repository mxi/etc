cmake_minimum_required(VERSION 3.16)

project(metal_basics LANGUAGES OBJC)
set(CMAKE_OBJC_STANDARD 11)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.5)

if(NOT APPLE)
    message(FATAL_ERROR "Cannot compile Metal applications on non-Apple devices.")
endif()

find_library(AppKit AppKit REQUIRED)
find_library(Metal Metal REQUIRED)
find_library(MetalKit MetalKit REQUIRED)
find_library(QuartzCore QuartzCore REQUIRED) # aka, CoreAnimation
message(STATUS "AppKit: ${AppKit}")
message(STATUS "Metal: ${Metal}")
message(STATUS "MetalKit: ${MetalKit}")
message(STATUS "QuartzCore: ${QuartzCore}")
link_libraries(${AppKit} ${Metal} ${MetalKit} ${QuartzCore})

if(NOT DEFINED METAL_TRIANGLE_RELEASE)
    set(SANITIZERS
        -fsanitize=undefined
        -fsanitize=address)
    add_compile_options(-Wall -g2 -Wextra -Wpedantic ${SANITIZERS})
    add_link_options(${SANITIZERS})
else()
    add_compile_options(-O2)
endif()

add_executable(triangle_colored triangle_colored.m)
add_executable(triangle_textured triangle_textured.m)
add_executable(triangle_textured_spinning triangle_textured_spinning.m)

add_executable(triangle_colored_ca MACOSX_BUNDLE
    triangle_colored_ca.m
    shaders/TriangleColored.metal)

set_source_files_properties(shaders/TriangleColored.metal PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/shaders")

set_target_properties(triangle_colored_ca PROPERTIES
    MACOSX_BUNDLE True
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.mxi.triangle_colored_ca
    MACOSX_BUNDLE_BUNDLE_NAME triangle_colored_ca
    MACOSX_BUNDLE_BUNDLE_VERSION "2025.09.10"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "2025.09.10"
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/stock/Info.plist.in
)
