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
#include "system\Lightmaps.hpp"

#include "other\GlmPrint.hpp"

/*
Bugs:
- Audio stops looping after a while
- Wrong bullet scaling coordinates
- Colliders as children
- Colliders on Transform add and remove
- Imgui keyboard input not working

General:
- Dynamic collision shape properties (scaling, weight, com)
- Raycasting from view (highlighting objects, moving axis object)
- Components/events for physics constraints/raycasting
- Hierarchy Helper (find by name, find in child, root, destroying children, removing parents)
- Threaded sound, mesh, and texture loading

Big:
- Audio system clean-up (seperate soundio/phonon (dsp) from libnyquist (sample loading))
- glLoader overaul (clean-up, mesh and texture glmapping, mesh/texture/shader objects)
- Renderer overhaul (deferred rendering, texture maps, light sources)
- Radiosity lightmap beginnings
*/

Game::Game(int argc, char** argv) : Engine(argc, argv){
	events.subscribe<PhysicsUpdateEvent>(*this);

	// Setup Test entities
	_spawnLocation = { 0, -1000, 90 };
	
	// Camera (body has collider, head has camera. Head is child of body)
	{	
		// Body (physics + z axis rotation)
		_body = entities.create();

		auto bodyTransform = _body.assign<Transform>();
		bodyTransform->position = _spawnLocation;

		Collider::BodyInfo bodyInfo;
		bodyInfo.type = Collider::Solid;
		bodyInfo.mass = 62; //kg
		bodyInfo.alwaysActive = true;
		bodyInfo.callbacks = true;
		bodyInfo.defaultAngularFactor = { 0, 0, 0 };
		//bodyInfo.defaultLinearFactor = { 1, 1, 0.5 };
		//bodyInfo.defaultLinearDamping = 0.9;
		//bodyInfo.defaultAngularDamping = 0.9;
		bodyInfo.defaultRestitution = 0;
		bodyInfo.defaultFriction = 0;

		auto bodyCollider = _body.assign<Collider>(Collider::ShapeInfo{ Collider::Capsule, 80, 150 }, bodyInfo);
			
		// Head (x axis rotation)
		_head = entities.create();

		auto headTransform = _head.assign<Transform>();
		headTransform->parent = _body;
		headTransform->position = { 0.f, 0.f, 150.f / 2.f };

		auto headCamera = _head.assign<Camera>();
		headCamera->verticalFov = 90.f;
		headCamera->zDepth = 500000.f;
		headCamera->offsetRotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });

		_head.assign<Listener>();

		// Set as controlled (enabled on focus event)
		systems.system<Controller>()->setControlled(_head, _body);
		systems.system<Controller>()->setEnabled(false);
	}

	// Floor
	{
		entityx::Entity plane = entities.create();

		auto transform = plane.assign<Transform>();
		transform->scale *= 50000;

		auto model = plane.assign<Model>(Model::FilePaths{ "shapes/plane.obj", 0, "grass.png" });
		model->textureScale *= 100;

		Collider::BodyInfo bodyInfo;
		bodyInfo.type = Collider::Static;
		bodyInfo.defaultRestitution = 1;

		auto collider = plane.assign<Collider>(Collider::ShapeInfo{ Collider::Plane }, bodyInfo);
	}

	// Skybox
	{	
		entityx::Entity skybox = entities.create();

		auto transform = skybox.assign<Transform>();
		transform->scale *= 5000;

		skybox.assign<Model>(Model::FilePaths{ "skybox.obj", 0, "skybox.png" });
	}

	// Testing triangle room
	{	
		entityx::Entity scene = entities.create();
	
		auto transform = scene.assign<Transform>();
		transform->position = { 0, 1000, 1 };
		transform->scale *= 0.8f;
		transform->rotation = glm::quat({ glm::radians(90.f), 0.f, 0.f });
	
		systems.system<Renderer>()->createScene(entities, "triangle_room.fbx", scene);
	}

	// Speakers
	{
		entityx::Entity speaker = entities.create();

		auto transform = speaker.assign<Transform>();
		transform->position = Transform::forward * 1000.f + Transform::up * 400.f;
		transform->rotation = glm::quat(glm::vec3(glm::half_pi<float>(), 0, 0));
		transform->scale *= 0.5f;

		speaker.assign<Model>(Model::FilePaths{ "speaker.obj", 0, "speaker.png" });

		Collider::BodyInfo bodyInfo;
		bodyInfo.type = Collider::Kinematic;
		bodyInfo.alwaysActive = true;

		speaker.assign<Collider>(Collider::ShapeInfo{ Collider::Box, 100, 100, 50 }, bodyInfo);

		Sound::Settings soundInfo;
		soundInfo.radius = 50000;
		soundInfo.falloffPower = 16;

		speaker.assign<Sound>("sounds/rain.wav", soundInfo);

		_spinners.push_back(speaker);
	}

	// Create platform
	{
		entityx::Entity platform = entities.create();

		auto transform = platform.assign<Transform>();
		transform->position = Transform::left * 1000.f + Transform::up * 100.f;
		transform->scale = { 100, 100, 10 };

		Collider::ShapeInfo shapeInfo;
		shapeInfo.type = Collider::Box;

		Collider::BodyInfo bodyInfo;
		bodyInfo.type = Collider::Kinematic;
		bodyInfo.defaultRestitution = 1;
		bodyInfo.defaultFriction = 1;
		bodyInfo.mass = 0;
		bodyInfo.alwaysActive = true;

		platform.assign<Collider>(shapeInfo, bodyInfo);

		platform.assign<Model>(Model::FilePaths{ "shapes/cube.obj", 0, "wood.png" });

		_spinners.push_back(platform);
	}
}

void Game::receive(const PhysicsUpdateEvent & physicsEvent){
	// Spin stuff for fun
	for (auto entity : _spinners) {
		if (!entity.has_component<Transform>())
			continue;

		auto transform = entity.component<Transform>();

		glm::quat rotation = glm::quat({ 0, 0, glm::radians(5 * physicsEvent.timestep) });

		transform->position = rotation * transform->position;
		transform->globalRotate(rotation);
	}
}

void Game::receive(const MousePressEvent& mousePressEvent){
	Engine::receive(mousePressEvent);

	if (mousePressEvent.action != Action::Press|| !_head.valid())
		return;

	auto headTransform = _head.component<Transform>();

	glm::vec3 globalHeadPosition;
	glm::quat globalHeadRotation;

	headTransform->globalDecomposed(&globalHeadPosition, &globalHeadRotation);

	glm::vec3 position = globalHeadRotation * Transform::forward * 200.f;
	glm::quat rotation = glm::quat({ 0.f, 0.f, glm::eulerAngles(globalHeadRotation).z });
	
	for (uint32_t i = 0; i < 1; i++) {
		switch (mousePressEvent.button) {
		case 1:
		{
			// Pizza box
			entityx::Entity testent = entities.create();

			auto transform = testent.assign<Transform>();
			transform->rotation = rotation;
			transform->position = globalHeadPosition + position + Transform::up * (float)i;
			transform->scale = { 100, 100, 10 };

			Collider::ShapeInfo shapeInfo;
			shapeInfo.type = Collider::Box;

			Collider::BodyInfo bodyInfo;
			bodyInfo.type = Collider::Solid;
			//bodyInfo.defaultRestitution = 1;
			bodyInfo.defaultFriction = 0.5;
			bodyInfo.mass = 5;
			bodyInfo.callbacks = true;

			testent.assign<Collider>(shapeInfo, bodyInfo);

			//Sound::Settings soundInfo;
			//soundInfo.playing = false;
			//soundInfo.loop = false;
			//soundInfo.radius = 25000;
			//soundInfo.falloffPower = 16;
			//
			//testent.assign<Sound>("sounds/box.wav", soundInfo);

			testent.assign<Model>(Model::FilePaths{ "shapes/cube.obj", 0, "pizza.png" });

			_sandbox.push_back(testent);
		}

		break;

		case 2:
		{
			// Anvil
			entityx::Entity testent = entities.create();

			auto transform = testent.assign<Transform>();
			transform->rotation = rotation;
			transform->position = globalHeadPosition + position + Transform::up * (float)i;
			transform->scale *= 100;

			Collider::BodyInfo bodyInfo;
			bodyInfo.type = Collider::Solid;
			bodyInfo.defaultRestitution = 0.25;
			bodyInfo.defaultFriction = 1;
			bodyInfo.defaultRollingFriction = 1;
			bodyInfo.defaultSpinningFriction = 1;
			//bodyInfo.defaultLinearDamping = 0.2;
			//bodyInfo.defaultAngularDamping = 0.2;
			bodyInfo.mass = 50000;
			bodyInfo.callbacks = true;

			testent.assign<Collider>(Collider::ShapeInfo{ Collider::Box, 1.5,1.5,1.5 }, bodyInfo);

			Sound::Settings soundInfo;
			soundInfo.loop = false;
			soundInfo.radius = 50000;
			soundInfo.falloffPower = 64;
			soundInfo.physical = true;
			soundInfo.playing = false;

			testent.assign<Sound>("sounds/thud.wav", soundInfo);

			testent.assign<Model>(Model::FilePaths{ "anvil.obj", 0, "anvil.png" });

			_sandbox.push_back(testent);
		}

		break;

		case 3:
		{
			// Beachball
			entityx::Entity testent = entities.create();

			auto transform = testent.assign<Transform>();
			transform->rotation = rotation;
			transform->position = globalHeadPosition + position + Transform::up * (float)i;
			transform->scale *= 64;

			Collider::ShapeInfo shapeInfo;
			shapeInfo.type = Collider::Sphere;

			Collider::BodyInfo bodyInfo;
			bodyInfo.type = Collider::Solid;
			bodyInfo.defaultRestitution = 0.95;
			bodyInfo.defaultFriction = 1;
			bodyInfo.defaultRollingFriction = 1;
			bodyInfo.defaultSpinningFriction = 1;
			bodyInfo.defaultLinearDamping = 0.2;
			bodyInfo.defaultAngularDamping = 0.2;
			bodyInfo.mass = 5;
			bodyInfo.callbacks = true;

			testent.assign<Collider>(shapeInfo, bodyInfo);

			Sound::Settings soundInfo;
			soundInfo.loop = false;
			soundInfo.radius = 20000;
			soundInfo.falloffPower = 32;
			soundInfo.physical = true;
			soundInfo.playing = false;

			testent.assign<Sound>("sounds/ball.wav", soundInfo);

			testent.assign<Model>(Model::FilePaths{ "shapes/sphere.obj", 0, "beachball.png" });

			_sandbox.push_back(testent);
		}

		break;
		}
	}
}

void Game::receive(const KeyInputEvent& keyInputEvent){
	Engine::receive(keyInputEvent);

	if (keyInputEvent.action != Action::Press || keyInputEvent.key != Key::Key_R)
		return;

	for (auto entity : _sandbox)
		entity.destroy();

	_sandbox.clear();
}

void Game::update(double dt){
	Engine::update(dt);

	// Buffer physics lines to renderer
	const BulletDebug& bulletDebug = systems.system<Physics>()->bulletDebug();
	systems.system<Renderer>()->rebufferLines(bulletDebug.lineCount(), bulletDebug.getLines());
}
