cmake_minimum_required(VERSION 3.0.0)
project(MetalAPI VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -fsanitize=address")

add_executable(app
src/00-window.cpp
)

target_include_directories(app PRIVATE dependencies/include/metal-cpp)
target_include_directories(app PRIVATE dependencies/include/metal-cpp-extensions)
target_link_libraries(app 
"-framework Metal"
"-framework Foundation"
"-framework Cocoa"
"-framework CoreGraphics"
"-framework MetalKit"
"-framework QuartzCore"
)
