#pragma once

#include <entityx\System.h>

#include <glad\glad.h>

#include <SDL.h>
#include <SDL_keyboard.h>

#include <glm\vec2.hpp>

class Window : public entityx::System<Window> {
	SDL_Window* _window = nullptr;
	SDL_GLContext _context = nullptr;

	bool _open = true;
	bool _contextWindow = true;

public:
	struct WindowInfo {
		uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
		const char* title = "";
		glm::uvec2 size = { 800, 600 };
		glm::uvec2 resolution = { 800, 600 };
		uint32_t monitor = 0;
		bool lockedCursor = false;
	};

	struct ConstructorInfo {
		uint32_t contextVersionMajor = 4;
		uint32_t contextVersionMinor = 6;
		bool debugContext = false;
		bool coreContex = true;

		WindowInfo defaultWindow;
	};

private:
	const ConstructorInfo _constructorInfo;
	WindowInfo _windowInfo;

	void _recreateWindow(entityx::EventManager & events);
	void _closeWindow(entityx::EventManager & events);

	void _pumpWindowEvents(entityx::EventManager & events);

public:
	Window(const ConstructorInfo& constructorInfo = ConstructorInfo());
	~Window();

	void update(entityx::EntityManager &entities, entityx::EventManager &events, double dt) final;

	void openWindow(const WindowInfo& windowInfo);
	void openWindow();
	void closeWindow();

	void lockCursor(bool lock);

	bool isOpen() const;
	WindowInfo windowInfo() const;
};