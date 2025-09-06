cmake_minimum_required(VERSION 3.30)

project(bernstein LANGUAGES C)
set(C_STANDARD_REQUIRED 99)
set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

set(MXI_SANITIZERS -fsanitize=undefined -fsanitize=address)
set(MXI_COMPILE_OPTIONS -O0 -g2 -Wall -Wextra ${MXI_SANITIZERS})
set(MXI_LINK_OPTIONS ${MXI_SANITIZERS})

add_executable(simple_demo simple_demo.c)
target_compile_options(simple_demo PRIVATE ${MXI_COMPILE_OPTIONS})
target_link_options(simple_demo PRIVATE ${MXI_LINK_OPTIONS})