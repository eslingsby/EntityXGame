cmake_minimum_required(VERSION 3.4)

project("glad" C CXX)

file(GLOB_RECURSE src "*.h" "*.c")

add_library("glad" STATIC ${src})

target_include_directories("glad" PUBLIC "include")