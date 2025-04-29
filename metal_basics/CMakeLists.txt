cmake_minimum_required(VERSION 3.16)

project(metal_basics LANGUAGES OBJC)
set(CMAKE_OBJC_STANDARD 11)
set(CMAKE_OBJC_STANDARD_REQUIRED ON)

if(NOT APPLE)
    message(FATAL_ERROR "Cannot compile Metal applications on non-Apple devices.")
endif()

find_library(AppKit AppKit REQUIRED)
find_library(Metal Metal REQUIRED)
find_library(MetalKit MetalKit REQUIRED)
message(STATUS "AppKit: ${AppKit}")
message(STATUS "Metal: ${Metal}")
message(STATUS "MetalKit: ${MetalKit}")
link_libraries(${AppKit} ${Metal} ${MetalKit})

if(NOT DEFINED METAL_TRIANGLE_RELEASE)
    set(SANITIZERS
        -fsanitize=undefined
        -fsanitize=address)
    add_compile_options(-Wall -Wextra -Wpedantic ${SANITIZERS})
    add_link_options(${SANITIZERS})
else()
    add_compile_options(-O2)
endif()

add_executable(triangle_colored triangle_colored.m)
add_executable(triangle_textured triangle_textured.m)
add_executable(triangle_textured_spinning triangle_textured_spinning.m)
