#include "Game.hpp"

#include "component\Transform.hpp"
#include "component\Collider.hpp"
#include "component\Model.hpp"
#include "component\Name.hpp"
#include "component\Listener.hpp"

#include "system\Window.hpp"
#include "system\Renderer.hpp"
#include "system\Controller.hpp"
#include "system\Physics.hpp"
#include "system\Audio.hpp"
#include "system\Interface.hpp"
//#include "system\Lightmaps.hpp"

#include "other\Time.hpp"

#include <glm\gtx\quaternion.hpp>

#include <experimental\filesystem>
#include <iostream>
#include <fstream>

/*
To-do:
- Audio for physics events
- Audio file sharing

- Maybe model should contain GlLoader* and collider contain btRigidBodyWorld* ???

- Fix bullet scaling coordinates
- Fix colliders as children
- Dynamic collision shape scaling / changing weight / properties

- Make sound loading threaded / job based
- Make mesh and texture loading threaded / job based

- Split into Engine.hpp (systems setup and integration) and Game.hpp (entity creation and testing stuff)

- Raycasting from view / moving axis object / Focus on click 
- Fix hover and keyboard input in imgui
- Imgui more component fields / Scene entity table

- Transform functions relative to parent
- Find by name, find in child, root / destroying children / removing parents

- Incoming events. I.e. build lightmap event / buffer debug lines event
*/

Game::Game(int argc, char** argv){
	srand(time(0));
	
	auto dataPath = std::experimental::filesystem::path(argv[0]).replace_filename("data/");
	
#ifndef _DEBUG
	auto errorLogPath = dataPath.parent_path();
	errorLogPath.replace_filename("log.txt");

	// closes file on exit, better solution later
	static std::ofstream cerrOut(errorLogPath.string());
	
	std::cerr.rdbuf(cerrOut.rdbuf());
#endif

	// Renderer system
	Renderer::ConstructorInfo rendererInfo;
	rendererInfo.path = dataPath.string();
	rendererInfo.mainVertexShader = "shaders/mainVert.glsl";
	rendererInfo.mainFragmentShader = "shaders/mainFrag.glsl";
	rendererInfo.lineVertexShader = "shaders/lineVert.glsl";
	rendererInfo.lineFragmentShader = "shaders/lineFrag.glsl";
	rendererInfo.defaultTexture = "checker.png";

	Window::ConstructorInfo windowInfo;
	//windowInfo.debugContext = true;
	windowInfo.defaultWindow.title = "EntityX Game";
	windowInfo.defaultWindow.lockedCursor = false;

	Physics::ConstructorInfo physicsInfo;
	physicsInfo.defaultGravity = { 0, 0, -9807 };
	physicsInfo.stepsPerUpdate = 4;
	physicsInfo.maxSubSteps = 0;

	Audio::ConstructorInfo audioInfo;
	audioInfo.sampleRate = 48000;
	audioInfo.frameSize = 512;
	audioInfo.path = dataPath.string();

	// Register systems
	systems.add<Window>(windowInfo);
	systems.add<Renderer>(rendererInfo);
	systems.add<Interface>();
	systems.add<Controller>();
	systems.add<Physics>(physicsInfo);
	systems.add<Audio>(audioInfo);
	//systems.add<Lightmaps>();

	systems.configure();

	// Register events
	events.subscribe<WindowFocusEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<WindowOpenEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);

	// Setup Test entities
	_spawnLocation = { 0, -5000, 0 };
	
	// Camera (body has collider, head has camera. Head is child of body)
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
		//bodyInfo.defaultLinearFactor = { 0, 0, 0 };
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

		_head.assign<Listener>();

		// Set as controlled (enabled on focus event)
		systems.system<Controller>()->setControlled(_head, _body);
		systems.system<Controller>()->setEnabled(false);

		// Set as focused UI entity
		systems.system<Interface>()->setFocusedEntity(_body);
	}

	// Testing sounds
	struct SoundInfo {
		std::string soundFile;
		float radius;
		uint32_t falloffPower;
	};

	std::vector<SoundInfo> soundInfos = {
		{ "sounds/rain.wav", 100000.f, 4 },
		{ "sounds/thunder.wav", 50000.f, 4 },
		{ "sounds/crows.wav", 50000.f, 4 }
	};

	float radius = 5000.f;

	uint32_t count = soundInfos.size();
	float stepDegrees = 360.f / count;

	for (uint32_t i = 0; i < count; i++) {		
		entityx::Entity sound = entities.create();

		auto transform = sound.assign<Transform>();
		transform->scale = { 2, 2, 2 };
		transform->rotation = glm::quat({ glm::radians(90.f + 45.f), 0, glm::radians(stepDegrees * i) });
		transform->position = glm::toMat4(glm::quat({ 0, 0, glm::radians(stepDegrees * i) })) * glm::vec4(Transform::forward * radius, 1);
		transform->position.z = 2000.f;

		sound.assign<Model>(Model::FilePaths{ "speaker.obj", 0, "speaker.png" });

		sound.assign<Sound>(soundInfos[i].soundFile, Sound::Settings{ soundInfos[i].radius, soundInfos[i].falloffPower });

		sound.assign<Collider>(Collider::ShapeInfo{ Collider::Box, 100, 100, 100 });

		_sounds.push_back(sound);
	}

	// Axis (with collider for each axis)
	{
		// Axis root
		entityx::Entity axis = entities.create();

		auto axisTransform = axis.assign<Transform>();
		axisTransform->position = { 0, 0, 1000 };
		axisTransform->scale = { 5, 5, 5 };
	
		// Axis visual
		entityx::Entity axisVisual = entities.create();
		
		auto axisVisualTransform = axisVisual.assign<Transform>();
		axisVisualTransform->parent = axis;
		axisVisualTransform->scale = { 50, 50, 50 };
	
		axisVisual.assign<Model>(Model::FilePaths{ "axis.obj", 0, "rgb.png" });

		// Shared variables
		glm::vec3 colliderScale = { 50, 50, 475 };

		Collider::ShapeInfo shapeInfo;
		shapeInfo.type = Collider::Cylinder;

		// X collider
		entityx::Entity xAxis = entities.create();

		auto xTransform = xAxis.assign<Transform>();
		xTransform->parent = axis;
		xTransform->rotation = glm::quat(glm::radians(glm::vec3{ 0.f, 90.f, 0.f }));
		xTransform->scale = colliderScale;
		xTransform->position = { colliderScale.z / 2, 0, 0 };

		//xAxis.assign<Model>(Model::FilePaths{ "shapes/cylinder.obj", 0, "rgb.png" });
		xAxis.assign<Collider>(shapeInfo);

		// Y collider
		entityx::Entity yAxis = entities.create();
		
		auto yTransform = yAxis.assign<Transform>();
		yTransform->parent = axis;
		yTransform->position = { 0, colliderScale.z / 2, 0 };
		yTransform->scale = colliderScale;
		yTransform->rotation = glm::quat(glm::radians(glm::vec3{ 90.f, 0.f, 0.f }));
		
		//yAxis.assign<Model>(Model::FilePaths{ "shapes/cylinder.obj", 0, "rgb.png" });
		yAxis.assign<Collider>(shapeInfo);
		
		// Z collider
		entityx::Entity zAxis = entities.create();
		
		auto zTransform = zAxis.assign<Transform>();
		zTransform->parent = axis;
		zTransform->position = { 0, 0, colliderScale.z / 2 };
		zTransform->scale = colliderScale;
		
		//zAxis.assign<Model>(Model::FilePaths{ "shapes/cylinder.obj", 0, "rgb.png" });
		zAxis.assign<Collider>(shapeInfo);
	}

	// Testing ball (child of head)
	//{
	//	entityx::Entity ball = entities.create();
	//
	//	ball.assign<Name>("test");
	//
	//	auto transform = ball.assign<Transform>();
	//	transform->parent = _head;
	//	transform->position = { 0.f, 1000.f, 0.f };
	//	transform->scale = { 100, 100, 100 };
	//
	//	ball.assign<Model>(Model::FilePaths{ "shapes/sphere.obj", 0, "rgb.png" });
	//
	//	Collider::BodyInfo bodyInfo;
	//	bodyInfo.type = Collider::Trigger;
	//	//bodyInfo.mass = 10;
	//	bodyInfo.alwaysActive = true;
	//	bodyInfo.callbacks = true;
	//
	//	ball.assign<Collider>(Collider::ShapeInfo{ Collider::Sphere }, bodyInfo);
	//}

	// Floor
	{
		entityx::Entity plane = entities.create();

		auto transform = plane.assign<Transform>();
		transform->position = { 0.f, 0.f, 0.f };
		transform->scale = { 1000000, 1000000, 1000000 };

		plane.assign<Model>(Model::FilePaths{ "shapes/plane.obj", 0, "checker.png" });

		auto collider = plane.assign<Collider>(Collider::ShapeInfo{ Collider::Plane });
	}

	// Skybox
	{	
		entityx::Entity skybox = entities.create();

		auto transform = skybox.assign<Transform>();
		transform->scale = { 50000, 50000, 50000 };

		skybox.assign<Model>(Model::FilePaths{ "skybox.obj", 0, "skybox.png" });
	}

	// Testing triangle room
	{	
		entityx::Entity scene = entities.create();
	
		auto transform = scene.assign<Transform>();
		transform->position = { 0, 10000, 100 };
		transform->scale = { 5, 5, 5 };
		transform->rotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });
	
		systems.system<Renderer>()->createScene(entities, "triangle_room.fbx", scene);
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

	// Create a bunch of junk on mouse events (testing)
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

	if (mousePressEvent.mods & Modifier::Mod_Ctrl) {
		switch (mousePressEvent.button) {
		case 1:
			// right mouse + ctrl (speaker)
			transform->scale *= 2;
			transform->rotation = glm::quat(glm::radians(glm::vec3{ rand() % 360, rand() % 360, rand() % 360 }));

			model = testent.assign<Model>(Model::FilePaths{ "speaker.obj", 0, "speaker.png" });

			bodyInfo.mass = 10;
			collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Cylinder, 100, 50, 50 }, bodyInfo);

			switch (rand() % 4) {
			case 0:
				testent.assign<Sound>("sounds/rain.wav", Sound::Settings{ 100000, 10 });
				return;
			case 1:
				testent.assign<Sound>("sounds/crows.wav", Sound::Settings{ 100000, 10 });
				return;
			case 2:
				testent.assign<Sound>("sounds/thunder.wav", Sound::Settings{ 100000, 10 });
				return;
			}

			return;

		case 2:
			return;

		case 3:
			return;
		}
	}


	switch (mousePressEvent.button) {
	case 1: 
		// left mouse (heavy weight)
		transform->scale = { 1000.f, 1000.f, 1000.f };
		model = testent.assign<Model>(Model::FilePaths{ "anvil.obj", 0, "anvil.png" });

		bodyInfo.mass = 10000;
		bodyInfo.defaultFriction = 1;
		bodyInfo.defaultRestitution = 0;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Box, 1.5f, 1.5f, 1.5f }, bodyInfo);

		return;

	case 2: 
		// middle mouse (bouncy ball)
		transform->scale = { 500.f, 500.f, 500.f };
		transform->rotation = glm::quat({ 0, 0, glm::radians((float)(rand() % 360)) });
		model = testent.assign<Model>(Model::FilePaths{ "shapes/sphere.obj", 0, "beachball.png" });

		bodyInfo.mass = 5;
		bodyInfo.defaultRestitution = 1.f;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Sphere }, bodyInfo);

		return;

	case 3: 
		// right mouse (light pizza box)
		transform->scale = { 700.f, 700.f, 100.f };
		model = testent.assign<Model>(Model::FilePaths{ "shapes/cube.obj", 0, "pizza.png" });

		bodyInfo.mass = 1;
		collider = testent.assign<Collider>(Collider::ShapeInfo{ Collider::Box }, bodyInfo);

		return;
	}
}

void Game::receive(const WindowOpenEvent& windowOpenEvent){
	if (!windowOpenEvent.opened)
		_running = false;
}

void Game::receive(const KeyInputEvent& keyInputEvent){
	switch (keyInputEvent.key) {
	case Key_Escape:
		// unlock mouse, or quit
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
		// clean-up
		if (keyInputEvent.action == Action::Press && !_rDown) {
		
			if (keyInputEvent.action != Action::Press)
				return;

			for (entityx::Entity& entity : _sandbox)
				entity.destroy();

			_sandbox.clear();

			_rDown = true;
		}
		else if (keyInputEvent.action == Action::Release && _rDown) {
			_rDown = false;
		}

		return;

	case Key_T:
		// respawn
		if (keyInputEvent.action == Action::Press && !_rDown) {
			if (keyInputEvent.action != Action::Press)
				return;

			if (!_body.valid() || !_body.has_component<Transform>())
				return;

			_body.component<Transform>()->position = _spawnLocation;

			if (!_body.has_component<Collider>())
				return;

			_body.component<Collider>()->setAngularVelocity({ 0, 0, 0 });
			_body.component<Collider>()->setLinearVelocity({ 0, 0, 0 });

			_tDown = true;
		}
		else if (keyInputEvent.action == Action::Release && _rDown) {
			_tDown = false;
		}

		return;
	}
}

int Game::run(){
	TimePoint timer;
	double dt = 0.0;
	
	while (_running) {
		startTime(&timer);
	
		systems.update_all(dt);
		
		// testing
		{
			// Buffer physics lines to renderer
			//const BulletDebug& bulletDebug = systems.system<Physics>()->bulletDebug();
			//systems.system<Renderer>()->rebufferLines(bulletDebug.lineCount(), bulletDebug.getLines());

			// Spin sounds for fun
			for (auto entity : _sounds) {
				if (!entity.has_component<Transform>())
					continue;
			
				auto transform = entity.component<Transform>();
			
				glm::mat4 rotation = glm::toMat4(glm::quat({ 0, 0, glm::radians(20 * dt) }));
			
				transform->position = rotation * glm::vec4(transform->position, 1);
			
				transform->globalRotate(rotation);
			}
		}

		dt = deltaTime(timer);
	}
	
	return _error;
}
