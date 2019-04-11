cmake_minimum_required(VERSION 3.4)

project("imgui" C CXX)

file(GLOB src "*.h" "*.cpp" "examples/imgui_impl_opengl3.h" "examples/imgui_impl_opengl3.cpp" "examples/imgui_impl_sdl.h" "examples/imgui_impl_sdl.cpp")

add_library("imgui" STATIC ${src})

target_link_libraries("imgui" "glad")
target_link_libraries("imgui" "SDL2main")
target_link_libraries("imgui" "SDL2-static")

target_compile_definitions("imgui" PUBLIC "IMGUI_IMPL_OPENGL_LOADER_GLAD")

target_include_directories("imgui" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")