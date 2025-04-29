cmake_minimum_required(VERSION 3.16)

project(objc_cpp_interop LANGUAGES OBJCXX CXX)
set(CMAKE_OBJCXX_STANDARD 11)
set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SANITIZERS
    -fsanitize=undefined
    -fsanitize=address)
add_compile_options(-Wall -Wextra -Wpedantic ${SANITIZERS})
add_link_options(${SANITIZERS})

add_library(iface iface.cpp)
# set_target_properties(iface PROPERTIES LANGUAGES CXX)

add_executable(main main.mm)
target_link_libraries(main iface)
