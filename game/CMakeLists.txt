file(GLOB src "*.hpp" "*.cpp" "*.h")

add_executable("Game" "${src}")

#target_link_libraries("Game" "Framework")

target_link_libraries("Game" "SDL2main")
target_link_libraries("Game" "SDL2-static")
target_link_libraries("Game" "glad")
target_link_libraries("Game" "glm")
target_link_libraries("Game" "assimp")
target_link_libraries("Game" "stb")
target_link_libraries("Game" "entityx")
target_link_libraries("Game" "sfml-audio")
target_link_libraries("Game" "LinearMath")
target_link_libraries("Game" "BulletCollision")
target_link_libraries("Game" "BulletDynamics")
target_link_libraries("Game" "imgui")
#target_link_libraries("Game" "RapidJSON")

#target_link_libraries("Game" winmm.lib)

if(MSVC)
	set_target_properties("Game" PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /entry:mainCRTStartup")
	set_target_properties("Game" PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS /entry:mainCRTStartup")
endif(MSVC)