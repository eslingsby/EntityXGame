#include "Engine.hpp"

#include "system\Window.hpp"
#include "system\Renderer.hpp"
#include "system\Controller.hpp"
#include "system\Physics.hpp"
#include "system\Audio.hpp"
#include "system\Interface.hpp"
#include "system\Lightmaps.hpp"

#include "other\Time.hpp"

#include <experimental\filesystem>
#include <iostream>
#include <fstream>

Engine::Engine(int argc, char** argv) {
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
	windowInfo.defaultWindow.title = "EntityX Engine";
	windowInfo.defaultWindow.lockedCursor = false;
	//windowInfo.defaultWindow.mode = Window::Windowed;
	//windowInfo.defaultWindow.size = { 1280, 720 };
	//windowInfo.debugContext = true;

	Physics::ConstructorInfo physicsInfo;
	physicsInfo.defaultGravity = { 0, 0, -980.7f };
	physicsInfo.stepsPerUpdate = 1;
	//physicsInfo.debugLines = true;

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
	systems.add<Lightmaps>();

	systems.configure();

	// Register events
	events.subscribe<WindowFocusEvent>(*this);
	events.subscribe<MousePressEvent>(*this);
	events.subscribe<WindowOpenEvent>(*this);
	events.subscribe<KeyInputEvent>(*this);
}

void Engine::receive(const WindowFocusEvent& windowFocusEvent) {
	if (!windowFocusEvent.focused && systems.system<Window>()->windowInfo().lockedCursor) {
		systems.system<Window>()->lockCursor(false);
		systems.system<Controller>()->setEnabled(false);
		systems.system<Interface>()->setInputEnabled(true);
	}
}

void Engine::receive(const MousePressEvent& mousePressEvent) {
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
}

void Engine::receive(const WindowOpenEvent& windowOpenEvent) {
	if (!windowOpenEvent.opened)
		_running = false;
}

void Engine::receive(const KeyInputEvent& keyInputEvent) {
	if (keyInputEvent.key != Key_Escape || keyInputEvent.action != Action::Press)
		return;

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
}

void Engine::update(double dt){
	systems.update_all(dt);
}

int Engine::run() {
	TimePoint timer;
	double dt = 0.0;

	while (_running) {
		startTime(&timer);

		update(dt);

		dt = deltaTime(timer);
	}

	return _error;
}
