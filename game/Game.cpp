#include "Game.hpp"


#include "Transform.hpp"
#include "Collider.hpp"
#include "Model.hpp"

#include "Window.hpp"
#include "Renderer.hpp"
#include "Controller.hpp"
#include "Physics.hpp"
#include "Audio.hpp"
#include "Interface.hpp"

#include <chrono>
#include <experimental\filesystem>
#include <optional>

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
- Colliders as children
- BtCollisionShape sharing

- Naming, find in child system / hierarchy helper

- Audio for physics events
*/

void _createTower(entityx::EntityManager& entities, std::vector<entityx::Entity>& sandbox, uint32_t count, float spin = 2.f, float width = 200000, float depth = 20000, float height = 10000) {
	for (uint32_t i = 0; i < count; i++) {
		entityx::Entity box = entities.create();
	
		auto transform = box.assign<Transform>();
		transform->position = { 0.f, 0.f, (height / 2.f) + height * i };
		transform->scale = { depth, width, height };
	
		transform->rotation = glm::quat({ 0.f, 0.f, glm::radians(i * (360.f / count) * spin) });
	
		box.assign<Model>(Model::FilePaths{ "shapes/cube.obj", 0, "wood.png" });
	
		auto collider = box.assign<Collider>(Collider::ShapeInfo{ Collider::Box }, Collider::BodyInfo{ Collider::Dynamic, 100 });
	
		//collider->setActive(false);

		sandbox.push_back(box);
	}
}

Game::Game(int argc, char** argv){
	auto path = std::experimental::filesystem::path(argv[0]).replace_filename("data/");

	// Renderer system
	Renderer::ConstructorInfo rendererInfo;
	rendererInfo.path = path.string();
	rendererInfo.defaultVertexShader = "vertexShader.glsl";
	rendererInfo.defaultFragmentShader = "fragmentShader.glsl";
	rendererInfo.defaultTexture = "checker.png";

	Window::ConstructorInfo windowInfo;
	//windowInfo.debugContext = true;
	windowInfo.defaultWindow.title = "EntityX Game";
	windowInfo.defaultWindow.lockedCursor = false;

	Physics::ConstructorInfo physicsInfo;
	physicsInfo.defaultGravity = { 0, 0, -5000 };
	physicsInfo.stepsPerUpdate = 20;
	physicsInfo.maxSubSteps = 0;

	// Register systems
	systems.add<Window>(windowInfo);
	systems.add<Renderer>(rendererInfo);
	systems.add<Interface>();

	systems.add<Controller>();
	systems.add<Physics>(physicsInfo);
	systems.add<Audio>();

	systems.configure();

	// Register events
	events.subscribe<WindowFocusEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<WindowOpenEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);

	// Setup Test entities
	_spawnLocation = { 0, -5000, 0 };
	
	// Camera
	{	
		// Body (physics + z axis rotation)
		_body = entities.create();

		auto bodyTransform = _body.assign<Transform>();
		bodyTransform->position = _spawnLocation;
		//bodyTransform->scale = { 500.f, 500.f, 1000.f };

		Collider::BodyInfo bodyInfo;
		bodyInfo.mass = 10;
		bodyInfo.alwaysActive = true;
		bodyInfo.callbacks = true;
		bodyInfo.defaultLinearFactor = { 0, 0, 0 };
		bodyInfo.defaultAngularFactor = { 0, 0, 0 };

		auto bodyCollider = _body.assign<Collider>(Collider::ShapeInfo{ Collider::Capsule, 600.f, 600.f }, bodyInfo);

		//auto bodyModel = _body.assign<Model>(Model::FilePaths{ "cube.obj", 0, "rgb.png" });

		// Head (x axis rotation)
		_head = entities.create();

		auto headTransform = _head.assign<Transform>();
		headTransform->parent = _body;
		headTransform->position = { 0.f, 0.f, 300.f };

		auto headCamera = _head.assign<Camera>();
		headCamera->verticalFov = 90.f;
		headCamera->zDepth = 4000000.f;
		headCamera->offsetRotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });

		// Set as controlled
		systems.system<Controller>()->setControlled(_head, _body);
		systems.system<Controller>()->setEnabled(false);

		// Set as focused UI entity
		systems.system<Interface>()->setFocusedEntity(_body);
	}

	// Testing ball
	//for (uint32_t i = 0; i < 2; i++){
	//	entityx::Entity ball = entities.create();
	//
	//	auto transform = ball.assign<Transform>();
	//	transform->scale = { 500, 500, 500 };
	//	transform->position = _spawnLocation + glm::vec3{ 0, 1000, 1000 + 1000 * i };
	//
	//	//transform->position = { 0, 10000, 500 };
	//	//transform->parent = _head;
	//			
	//	auto model = ball.assign<Model>(Model::FilePaths{ "shapes/sphere.obj", 0, "rgb.png" });
	//
	//	ball.assign<Collider>(Collider::ShapeInfo{ Collider::Sphere }, Collider::BodyInfo{ Collider::Dynamic, 10.f });
	//}

	// Helix tower (for fun)
	//_createTower(entities, _sandbox, 50, 10, 10000, 5000, 1000);

	// Testing triangle room
	//{	
	//	entityx::Entity scene = entities.create();
	//
	//	auto transform = scene.assign<Transform>();
	//	transform->scale = { 5, 5, 5 };
	//	transform->rotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });
	//
	//	systems.system<Renderer>()->createScene(entities, "triangle_room.fbx", scene);
	//}

	// Axis
	{
		entityx::Entity axis = entities.create();
	
		auto transform = axis.assign<Transform>();
		transform->scale = { 50, 50, 50 };
	
		//axis.assign<Model>(Model::FilePaths{ "axis.obj", 0, "rgb.png" });
	}

	// Floor
	{
		entityx::Entity plane = entities.create();

		auto transform = plane.assign<Transform>();
		transform->position = { 0.f, 0.f, 0.f };
		transform->scale = { 1000000, 1000000, 1000000 };

		plane.assign<Model>(Model::FilePaths{ "shapes/plane.obj", 0, "checker.png" });

		Collider::BodyInfo bodyInfo;
		bodyInfo.type = Collider::Static;
		//bodyInfo.defaultRestitution = 1.f;
		bodyInfo.mass = 0;

		auto collider = plane.assign<Collider>(Collider::ShapeInfo{ Collider::Plane }, bodyInfo);
	}

	// Skybox
	{	
		entityx::Entity skybox = entities.create();

		auto transform = skybox.assign<Transform>();
		transform->scale = { 50000, 50000, 50000 };

		skybox.assign<Model>(Model::FilePaths{ "skybox.obj", 0, "skybox.png" });
	}
}

void Game::receive(const WindowFocusEvent& windowFocusEvent){
	if (!windowFocusEvent.focused && systems.system<Window>()->windowInfo().lockedCursor) {
		systems.system<Window>()->lockCursor(false);
		systems.system<Controller>()->setEnabled(false);
		systems.system<Interface>()->setInputEnabled(true);
	}
}

void Game::receive(const MousePressEvent& mousePressEvent){
	if (mousePressEvent.action == Action::Press && systems.system<Interface>()->isHovering()) {
		_wasHovering = true;
		return;
	}
	else if (systems.system<Interface>()->isHovering()) {
		return;
	}

	if (_wasHovering && mousePressEvent.action == Action::Release) {
		_wasHovering = false;
		return;
	}

	if (!systems.system<Window>()->windowInfo().lockedCursor) {
		systems.system<Window>()->lockCursor(true);
		systems.system<Controller>()->setEnabled(true);
		systems.system<Interface>()->setInputEnabled(false);
		return;
	}

	if (mousePressEvent.action != Action::Press)
		return;

	entityx::Entity testent = entities.create();
	_sandbox.push_back(testent);

	systems.system<Interface>()->setFocusedEntity(testent);

	auto cameraTransform = _head.component<Transform>();

	glm::vec3 globalPosition;
	glm::quat globalRotation;
	cameraTransform->globalDecomposed(&globalPosition, &globalRotation);
	
	auto transform = testent.assign<Transform>();
	transform->position = globalPosition + globalRotation * Transform::forward * 2500.f;
	transform->rotation = glm::quat({ 0, 0, glm::eulerAngles(globalRotation).z });
	
	entityx::ComponentHandle<Collider> collider;
	entityx::ComponentHandle<Model> model;

	Collider::BodyInfo bodyInfo;

	switch (mousePressEvent.button) {
	case 1: // left mouse
		transform->scale = { 1000.f, 1000.f, 1000.f };
		model = testent.assign<Model>(Model::FilePaths{ "anvil.obj", 0, "anvil.png" });

		bodyInfo.mass = 10000;
		bodyInfo.defaultFriction = 1;
		bodyInfo.defaultRestitution = 0;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Box, 1.5f, 1.5f, 1.5f }, bodyInfo);
		break;

	case 2: // middle mouse
		transform->scale = { 500.f, 500.f, 500.f };
		model = testent.assign<Model>(Model::FilePaths{ "shapes/sphere.obj", 0, "rgb.png" });

		bodyInfo.mass = 5;
		bodyInfo.defaultRestitution = 1.f;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Sphere }, bodyInfo);
		break;

	case 3: // right mouse
		transform->scale = { 700.f, 700.f, 100.f };
		model = testent.assign<Model>(Model::FilePaths{ "shapes/cube.obj", 0, "pizza.png" });

		bodyInfo.mass = 1;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Box }, bodyInfo);
		break;
	}
}

void Game::receive(const WindowOpenEvent& windowOpenEvent){
	if (!windowOpenEvent.opened)
		_running = false;
}

void Game::receive(const KeyInputEvent& keyInputEvent){
	switch (keyInputEvent.key) {
	case Key_Escape:
		if (keyInputEvent.action != Action::Press)
			return;

		if (systems.system<Window>()->windowInfo().lockedCursor) {
			systems.system<Window>()->lockCursor(false);
			systems.system<Controller>()->setEnabled(false);
			systems.system<Interface>()->setInputEnabled(true);
		}
		else {
			_running = false;
		}

		return;

	case Key_R:
		if (keyInputEvent.action != Action::Release)
			return;

		for (entityx::Entity& entity : _sandbox)
			entity.destroy();

		_sandbox.clear();
		
		//_createTower(entities, _sandbox, 100, 10, 10000, 5000, 500);

		return;

	case Key_T:
		if (keyInputEvent.action != Action::Release)
			return;

		if (!_body.valid() || !_body.has_component<Transform>())
			return;

		_body.component<Transform>()->position = _spawnLocation;

		if (!_body.has_component<Collider>())
			return;

		_body.component<Collider>()->setAngularVelocity({ 0, 0, 0 });
		_body.component<Collider>()->setLinearVelocity({ 0, 0, 0 });
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
