cmake_minimum_required(VERSION 3.30)

project(macos_basic_app LANGUAGES CXX OBJC)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

find_library(Foundation Foundation REQUIRED)
find_library(AppKit AppKit REQUIRED)

set(MXI_SANITIZERS -fsanitize=undefined -fsanitize=address)
set(MXI_COMPILE_OPTIONS -O0 -g2 -Wall -Wextra ${MXI_SANITIZERS})
set(MXI_LINK_OPTIONS ${MXI_SANITIZERS})

add_executable(macos_basic_app 
    src/main.m)

target_compile_options(macos_basic_app PRIVATE ${MXI_COMPILE_OPTIONS})
target_link_options(macos_basic_app PRIVATE ${MXI_LINK_OPTIONS})
target_link_libraries(macos_basic_app PRIVATE ${Foundation} ${AppKit})