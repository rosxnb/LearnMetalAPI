cmake_minimum_required(VERSION 3.27)
project(MetalAPI VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -fsanitize=address")

add_executable(MyMetalApp
src/main.cpp
src/app_delegate.cpp
src/view_delegate.cpp
src/renderer.cpp
src/utility.cpp
)

target_include_directories(MyMetalApp PRIVATE dependencies/include/metal-cpp)
target_include_directories(MyMetalApp PRIVATE dependencies/include/metal-cpp-extensions)
target_link_libraries(MyMetalApp 
"-framework Metal"
"-framework Foundation"
"-framework Cocoa"
"-framework CoreGraphics"
"-framework MetalKit"
"-framework QuartzCore"
)
