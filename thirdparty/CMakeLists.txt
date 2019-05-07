add_custom_target("uninstall")
set_target_properties("uninstall" PROPERTIES FOLDER "CMakePredefinedTargets")

option(BUILD_SHARED_LIBS "" OFF)

############# GLAD #############
add_subdirectory("glad-gl-4.6")

set_target_properties("glad" PROPERTIES FOLDER "Thirdparty")

############# GLM #############
option(GLM_STATIC_LIBRARY_ENABLE "" ON)

add_external_tar("glm-0.9.8.5" "https://github.com/g-truc/glm/archive/0.9.8.5.tar.gz" "")

set_target_properties("glm_static" PROPERTIES FOLDER "Thirdparty")

############# stb #############
add_subdirectory("stb")

set_target_properties("stb_dummy" PROPERTIES FOLDER "Thirdparty")

############# Assimp #############
option(ASSIMP_BUILD_TESTS "" OFF)
option(ASSIMP_BUILD_SAMPLES "" OFF)
option(ASSIMP_BUILD_SAMPLES "" OFF)

add_external_tar("assimp-4.1.0" "https://github.com/assimp/assimp/archive/v4.1.0.tar.gz" "")

target_include_directories("assimp" PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/assimp-4.1.0/include"
	"${CMAKE_BINARY_DIR}/thirdparty/assimp-4.1.0/include"
)

set_target_properties("assimp" PROPERTIES FOLDER "Thirdparty")
set_target_properties("assimp_cmd" PROPERTIES FOLDER "Thirdparty")
set_target_properties("IrrXML" PROPERTIES FOLDER "Thirdparty")
set_target_properties("UpdateAssimpLibsDebugSymbolsAndDLLs" PROPERTIES FOLDER "Thirdparty")
set_target_properties("zlib" PROPERTIES FOLDER "Thirdparty")
set_target_properties("zlibstatic" PROPERTIES FOLDER "Thirdparty")

target_compile_options("assimp" PUBLIC "/wd4996")
target_compile_options("assimp" PUBLIC "/wd4700")

############# SDL2 #############
add_external_tar("SDL2-2.0.8" "https://www.libsdl.org/release/SDL2-2.0.8.tar.gz" "")

target_include_directories("SDL2main" PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/SDL2-2.0.8/include>"
	"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/SDL2-2.0.8/include>"
)

set_target_properties("SDL2" PROPERTIES FOLDER "Thirdparty")
set_target_properties("SDL2-static" PROPERTIES FOLDER "Thirdparty")
set_target_properties("SDL2main" PROPERTIES FOLDER "Thirdparty")

############# EntityX #############
option(ENTITYX_BUILD_SHARED "" OFF)
option(ENTITYX_BUILD_TESTING "" OFF)

#add_external_tar("entityx-1.1.2" "https://github.com/alecthomas/entityx/archive/1.1.2.tar.gz" "")
add_external_tar("alecthomas-entityx-8262569" "https://codeload.github.com/alecthomas/entityx/legacy.tar.gz/8262569ed32a0c3a21321967cd61e6be8a6a90fb" "")

target_include_directories("entityx" PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/entityx-1.1.2>"
	"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/entityx-1.1.2>"
)

set_target_properties("entityx" PROPERTIES FOLDER "Thirdparty")

############# Bullet3 #############
option(BUILD_BULLET2_DEMOS "" OFF)
option(BUILD_CLSOCKET "" OFF)
option(BUILD_CPU_DEMOS "" OFF)
option(BUILD_ENET "" OFF)
option(BUILD_EXTRAS "" OFF)
option(BUILD_OPENGL3_DEMOS "" OFF)
option(BUILD_UNIT_TESTS "" OFF)
option(USE_MSVC_RUNTIME_LIBRARY_DLL "" ON)

option(USE_GLUT "" OFF)
option(USE_GRAPHICAL_BENCHMARK "" OFF)

add_external_tar("bullet3-2.88" "https://github.com/bulletphysics/bullet3/archive/2.88.tar.gz" "")

set("BULLET_SRC" "${CMAKE_CURRENT_SOURCE_DIR}/bullet3-2.88/src")

target_include_directories("BulletCollision" PUBLIC "${BULLET_SRC}")
target_include_directories("BulletDynamics" PUBLIC "${BULLET_SRC}")
target_include_directories("LinearMath" PUBLIC "${BULLET_SRC}")

set_target_properties("Bullet2FileLoader" PROPERTIES FOLDER "Thirdparty")
set_target_properties("Bullet3Collision" PROPERTIES FOLDER "Thirdparty")
set_target_properties("Bullet3Common" PROPERTIES FOLDER "Thirdparty")
set_target_properties("Bullet3Dynamics" PROPERTIES FOLDER "Thirdparty")
set_target_properties("Bullet3Geometry" PROPERTIES FOLDER "Thirdparty")
set_target_properties("Bullet3OpenCL_clew" PROPERTIES FOLDER "Thirdparty")
set_target_properties("BulletCollision" PROPERTIES FOLDER "Thirdparty")
set_target_properties("BulletDynamics" PROPERTIES FOLDER "Thirdparty")
set_target_properties("BulletInverseDynamics" PROPERTIES FOLDER "Thirdparty")
set_target_properties("BulletSoftBody" PROPERTIES FOLDER "Thirdparty")
set_target_properties("LinearMath" PROPERTIES FOLDER "Thirdparty")

target_compile_options("BulletDynamics" PUBLIC "/wd5033")
target_compile_options("BulletDynamics" PUBLIC "/wd4305")
target_compile_options("BulletCollision" PUBLIC "/wd4305")
target_compile_options("BulletCollision" PUBLIC "/wd5033")
target_compile_options("LinearMath" PUBLIC "/wd5033")
target_compile_options("LinearMath" PUBLIC "/wd4305")

############# ImGui #############
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.69/imgui.h")
	download_extract_tar("imgui-1.69" "https://github.com/ocornut/imgui/archive/v1.69.tar.gz")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/imgui-1.69/")

set_target_properties("imgui" PROPERTIES FOLDER "Thirdparty")

############# libsoundio #############
option(BUILD_DYNAMIC_LIBS "" OFF)
option(BUILD_STATIC_LIBS "" ON)

option(BUILD_EXAMPLE_PROGRAMS "" OFF)
option(BUILD_TESTS "" OFF)

add_external_tar("libsoundio-2.0.0" "https://github.com/andrewrk/libsoundio/archive/2.0.0.tar.gz" "")

target_include_directories("libsoundio_static" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/libsoundio-2.0.0")

target_compile_definitions("libsoundio_static" PUBLIC "SOUNDIO_STATIC_LIBRARY")

set_target_properties("libsoundio_static" PROPERTIES FOLDER "Thirdparty")
set_target_properties("doc" PROPERTIES FOLDER "Thirdparty")

############# Steam audio #############
if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/steamaudio_api/include/phonon.h")
	download_extract_tar("steamaudio_api" "https://github.com/ValveSoftware/steam-audio/releases/download/v2.0-beta.17/steamaudio_api_2.0-beta.17.zip")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/steamaudio_api/")

if (MSVC)
	if("${CMAKE_GENERATOR}" MATCHES "(Win64|IA64)")
		configure_file("steamaudio_api/bin/Windows/x64/phonon.dll" "${PROJECT_SOURCE_DIR}/bin" COPYONLY)
	else()
		configure_file("steamaudio_api/bin/Windows/x86/phonon.dll" "${PROJECT_SOURCE_DIR}/bin" COPYONLY)
	endif()
endif()

############# libnyquist #############
option(BUILD_EXAMPLE "" OFF)

add_external_tar("ddiakopoulos-libnyquist-ae0f455" "https://codeload.github.com/ddiakopoulos/libnyquist/legacy.tar.gz/ae0f4556150ced49615c75a0b9e8d6cf11ab50bd" "")

target_include_directories("libnyquist" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ddiakopoulos-libnyquist-ae0f455/include")

set_target_properties("libnyquist" PROPERTIES FOLDER "Thirdparty")
set_target_properties("libopus" PROPERTIES FOLDER "Thirdparty")
set_target_properties("libwavpack" PROPERTIES FOLDER "Thirdparty")

############# RapidJson #############
#option(RAPIDJSON_BUILD_DOC "" OFF)
#option(RAPIDJSON_BUILD_EXAMPLES "" OFF)
#option(RAPIDJSON_BUILD_TESTS "" OFF)
#
#add_external_tar("rapidjson-1.1.0" "https://github.com/Tencent/rapidjson/archive/v1.1.0.tar.gz" "")
#
#add_library("RapidJSON" INTERFACE)
#
#target_include_directories("RapidJSON" INTERFACE "rapidjson-1.1.0/include/")
#
#file(GLOB_RECURSE src "rapidjson-1.1.0/include/*.h")
#
#add_library("RapidJSON_dummy" STATIC ${src})
#set_target_properties("RapidJSON_dummy" PROPERTIES LINKER_LANGUAGE CXX)
#
#set_target_properties("travis_doc" PROPERTIES FOLDER "Thirdparty")
#set_target_properties("RapidJSON_dummy" PROPERTIES FOLDER "Thirdparty")