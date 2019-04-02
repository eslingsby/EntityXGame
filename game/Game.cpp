#include "Game.hpp"

#include "Window.hpp"
#include "Renderer.hpp"
#include "Controller.hpp"
#include "Transform.hpp"
#include "Collider.hpp"
#include "Physics.hpp"

#include <chrono>
#include <experimental\filesystem>

using Clock = std::chrono::high_resolution_clock;
using TimePoint = Clock::time_point;

inline void startTime(TimePoint* point) {
	*point = Clock::now();
}

template <typename T = double>
inline T deltaTime(const TimePoint& point) {
	return std::chrono::duration_cast<std::chrono::duration<T>>(Clock::now() - point).count();
}

/*
- Naming / Find in child

- Implement physics with bullet
- Implement audio with SFML
- Implement loading/saving with rapidjson

- Get a capsule running around in an enviorenment with a 3rd person camera!
*/

Game::Game(int argc, char** argv){
	auto path = std::experimental::filesystem::path(argv[0]).replace_filename("data/");

	// Renderer system
	Renderer::ConstructorInfo rendererInfo;
	rendererInfo.path = path.string();
	rendererInfo.defaultVertexShader = "vertexShader.glsl";
	rendererInfo.defaultFragmentShader = "fragmentShader.glsl";
	rendererInfo.defaultTexture = "checker.png";
	rendererInfo.defualtShape.zDepth = 100000.f;

	Window::ConstructorInfo windowInfo;
	windowInfo.debugContext = true;
	windowInfo.defaultWindow.title = "EntityX Game";
	windowInfo.defaultWindow.lockedCursor = false;

	// Register systems
	systems.add<Renderer>(rendererInfo);
	systems.add<Window>(windowInfo);
	systems.add<Physics>();
	systems.add<Controller>();

	systems.configure();

	// Register events
	events.subscribe<WindowFocusEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<WindowOpenEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);
	
	// Camera
	{	
		_camera = entities.create();

		auto transform = _camera.assign<Transform>();
		transform->position = { 0.f, -100.f, 0.f };
		transform->rotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });

		_camera.assign<Collider>(_camera, Collider::ShapeInfo{ Collider::Sphere, 10.f }, Collider::RigidInfo{ { 0.f, 0.f, 0.f }, 0.f, true });

		systems.system<Renderer>()->setCamera(_camera);
	}
	
	// Axis
	for (uint32_t i = 0; i < 100; i++){	
		entityx::Entity box = entities.create();

		auto transform = box.assign<Transform>();
		transform->position = { 0.f, 0.f, 10.f * i };
		transform->scale = { 10.f, 10.f, 10.f };

		box.assign<Model>(Model::FilePaths{ "axis.obj", 0, "rgb.png" });

		auto collider = box.assign<Collider>(box, Collider::ShapeInfo{ Collider::Sphere, 10.f }, Collider::RigidInfo{ {0.f, 0.f, 0.f}, 10.f });
	}

	// Plane
	{
		entityx::Entity plane = entities.create();

		auto transform = plane.assign<Transform>();
		transform->position = { 0.f, 0.f, -100.f };
		transform->scale = { 10000.f, 10000.f, 10000.f };

		plane.assign<Model>(Model::FilePaths{ "plane.obj", 0, "checker.png" });

		plane.assign<Collider>(plane, Collider::ShapeInfo{ Collider::Plane, 0.f, 0.f, 1.f }, Collider::RigidInfo{ { 0.f, 0.f, -10.f }, 0.f });
	}

	// Skybox
	{	
		entityx::Entity skybox = entities.create();

		auto transform = skybox.assign<Transform>();
		transform->scale = { 1000.f, 1000.f, 1000.f };

		skybox.assign<Model>(Model::FilePaths{ "skybox.obj", 0, "skybox.png" });
	}

	// Triangle room
	/*{	
		entityx::Entity scene = entities.create();

		auto transform = scene.assign<Transform>();
		//transform->scale = { 10.f, 10.f, 10.f };
		transform->rotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });

		systems.system<Renderer>()->createScene(entities, "triangle_room.fbx", scene);
	}*/
}

void Game::receive(const WindowFocusEvent& windowFocusEvent){
	if (!windowFocusEvent.focused && systems.system<Window>()->windowInfo().lockedCursor) {
		systems.system<Window>()->lockCursor(false);
		systems.system<Controller>()->setControlled(entityx::Entity());
	}
}

void Game::receive(const MousePressEvent& mousePressEvent){
	if (!systems.system<Window>()->windowInfo().lockedCursor) {
		systems.system<Window>()->lockCursor(true);
		systems.system<Controller>()->setControlled(_camera);
	}
}

void Game::receive(const WindowOpenEvent& windowOpenEvent){
	if (!windowOpenEvent.opened)
		_running = false;
}

void Game::receive(const KeyInputEvent& keyInputEvent){
	if (keyInputEvent.key == Key_Escape && keyInputEvent.action == Action::Press) {
		if (systems.system<Window>()->windowInfo().lockedCursor) {
			systems.system<Controller>()->setControlled(entityx::Entity());
			systems.system<Window>()->lockCursor(false);
		}
		else {
			_running = false;
		}
	}
}

int Game::run(){
	TimePoint timer;
	double dt = 0.0;
	
	while (_running) {
		startTime(&timer);
	
		systems.update_all(dt);
	
		dt = deltaTime(timer);
	}
	
	return _error;
}
