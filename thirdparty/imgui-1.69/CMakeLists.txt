cmake_minimum_required(VERSION 3.4)

project("imgui" C CXX)

file(GLOB src "*.h" "*.cpp")

add_library("imgui" STATIC ${src})

target_include_directories("imgui" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")