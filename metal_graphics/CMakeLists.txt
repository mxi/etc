cmake_minimum_required(VERSION 4.0)

project(metal_graphics LANGUAGES C OBJC)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_OBJC_STANDARD 11)
set(CMAKE_OBJC_STANDARD_REQUIRED YES)
set(CMAKE_OSX_DEPLOYMENT_TARGET 13.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

if(NOT APPLE)
    message(FATAL_ERROR "Cannot compile Metal applications on non-Apple devices.")
endif()

find_library(AppKit AppKit REQUIRED)
find_library(Metal Metal REQUIRED)
find_library(QuartzCore QuartzCore REQUIRED) # aka, CoreAnimation

set(MXI_SANITIZERS -fsanitize=undefined -fsanitize=address)
set(MXI_COMPILE_OPTIONS -Wall -Wextra -O0 -g2 -fno-strict-aliasing ${MXI_SANITIZERS})
set(MXI_LINK_OPTIONS ${MXI_SANITIZERS})

# add_library(mgapp STATIC
#     src/app/main.c
#     src/app/draw.c)
# target_include_directories(mgapp PRIVATE "src/platform")
# target_compile_options(mgapp PRIVATE ${MXI_COMPILE_OPTIONS})
# target_link_options(mgapp PRIVATE ${MXI_LINK_OPTIONS})

set(MXI_MG_SOURCES
    src/main.c)

set(MXI_MG_MACOS_SOURCES
    src/platform/macos/common.m
    src/platform/macos/main.m)

add_executable(mg MACOSX_BUNDLE
    ${MXI_MG_SOURCES}
    ${MXI_MG_MACOS_SOURCES})
target_compile_options(mg PRIVATE ${MXI_COMPILE_OPTIONS})
target_link_options(mg PRIVATE ${MXI_LINK_OPTIONS})
target_link_libraries(mg PRIVATE ${AppKit} ${Metal} ${QuartzCore})

# add_executable(triangle_colored_ca MACOSX_BUNDLE
#     triangle_colored_ca.m
#     shaders/TriangleColored.metal)
# 
# set_source_files_properties(shaders/TriangleColored.metal PROPERTIES
#     MACOSX_PACKAGE_LOCATION "Resources/shaders")
# 
# set_target_properties(triangle_colored_ca PROPERTIES
#     MACOSX_BUNDLE True
#     MACOSX_BUNDLE_GUI_IDENTIFIER io.github.mxi.triangle_colored_ca
#     MACOSX_BUNDLE_BUNDLE_NAME triangle_colored_ca
#     MACOSX_BUNDLE_BUNDLE_VERSION "2025.09.10"
#     MACOSX_BUNDLE_SHORT_VERSION_STRING "2025.09.10"
#     MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/stock/Info.plist.in
# )