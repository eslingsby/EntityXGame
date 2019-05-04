#include "system\Window.hpp"
#include "system\WindowEvents.hpp"

const std::unordered_map<uint32_t, uint32_t> sdlKeymap{
	{ SDLK_UNKNOWN, Key_Unknown },
	{ SDLK_SPACE, Key_Space },
	{ SDLK_QUOTE, Key_Quote },
	{ SDLK_COMMA, Key_Comma },
	{ SDLK_MINUS, Key_Minus },
	{ SDLK_PERIOD, Key_Period },
	{ SDLK_SLASH, Key_Slash },
	{ SDLK_0, Key_0 },
	{ SDLK_1, Key_1 },
	{ SDLK_2, Key_2 },
	{ SDLK_3, Key_3 },
	{ SDLK_4, Key_4 },
	{ SDLK_5, Key_5 },
	{ SDLK_6, Key_6 },
	{ SDLK_7, Key_7 },
	{ SDLK_8, Key_8 },
	{ SDLK_9, Key_9 },
	{ SDLK_SEMICOLON, Key_Semicolon },
	{ SDLK_EQUALS, Key_Equal },
	{ SDLK_a, Key_A },
	{ SDLK_b, Key_B },
	{ SDLK_c, Key_C },
	{ SDLK_d, Key_D },
	{ SDLK_e, Key_E },
	{ SDLK_f, Key_F },
	{ SDLK_g, Key_G },
	{ SDLK_h, Key_H },
	{ SDLK_i, Key_I },
	{ SDLK_j, Key_J },
	{ SDLK_k, Key_K },
	{ SDLK_l, Key_L },
	{ SDLK_m, Key_M },
	{ SDLK_n, Key_N },
	{ SDLK_o, Key_O },
	{ SDLK_p, Key_P },
	{ SDLK_q, Key_Q },
	{ SDLK_r, Key_R },
	{ SDLK_s, Key_S },
	{ SDLK_t, Key_T },
	{ SDLK_u, Key_U },
	{ SDLK_v, Key_V },
	{ SDLK_w, Key_W },
	{ SDLK_x, Key_X },
	{ SDLK_y, Key_Y },
	{ SDLK_z, Key_Z },
	{ SDLK_LEFTBRACKET, Key_LBracket },
	{ SDLK_BACKSLASH, Key_Backslash },
	{ SDLK_RIGHTBRACKET, Key_RBracket },
	{ SDLK_BACKQUOTE, Key_GraveAccent },
	{ SDLK_ESCAPE, Key_Escape },
	{ SDLK_RETURN, Key_Enter },
	{ SDLK_TAB, Key_Tab },
	{ SDLK_BACKSPACE, Key_Backspace },
	{ SDLK_INSERT, Key_Insert },
	{ SDLK_DELETE, Key_Delete },
	{ SDLK_RIGHT, Key_Right },
	{ SDLK_LEFT, Key_Left },
	{ SDLK_DOWN, Key_Down },
	{ SDLK_UP, Key_Up },
	{ SDLK_PAGEUP, Key_PageUp },
	{ SDLK_PAGEDOWN, Key_PageDown },
	{ SDLK_HOME, Key_Home },
	{ SDLK_END, Key_End },
	{ SDLK_CAPSLOCK, Key_CapsLock },
	{ SDLK_SCROLLLOCK, Key_ScrollLock },
	{ SDLK_NUMLOCKCLEAR, Key_NumLock },
	{ SDLK_PRINTSCREEN, Key_PrintScreen },
	{ SDLK_PAUSE, Key_Pause },
	{ SDLK_F1, Key_F1 },
	{ SDLK_F2, Key_F2 },
	{ SDLK_F3, Key_F3 },
	{ SDLK_F4, Key_F4 },
	{ SDLK_F5, Key_F5 },
	{ SDLK_F6, Key_F6 },
	{ SDLK_F7, Key_F7 },
	{ SDLK_F8, Key_F8 },
	{ SDLK_F9, Key_F9 },
	{ SDLK_F10, Key_F10 },
	{ SDLK_F11, Key_F11 },
	{ SDLK_F12, Key_F12 },
	{ SDLK_F13, Key_F13 },
	{ SDLK_F14, Key_F14 },
	{ SDLK_F15, Key_F15 },
	{ SDLK_F16, Key_F16 },
	{ SDLK_F17, Key_F17 },
	{ SDLK_F18, Key_F18 },
	{ SDLK_F19, Key_F19 },
	{ SDLK_F20, Key_F20 },
	{ SDLK_F21, Key_F21 },
	{ SDLK_F22, Key_F22 },
	{ SDLK_F23, Key_F23 },
	{ SDLK_F24, Key_F24 },
	{ SDLK_KP_0, Key_Num0 },
	{ SDLK_KP_1, Key_Num1 },
	{ SDLK_KP_2, Key_Num2 },
	{ SDLK_KP_3, Key_Num3 },
	{ SDLK_KP_4, Key_Num4 },
	{ SDLK_KP_5, Key_Num5 },
	{ SDLK_KP_6, Key_Num6 },
	{ SDLK_KP_7, Key_Num7 },
	{ SDLK_KP_8, Key_Num8 },
	{ SDLK_KP_9, Key_Num9 },
	{ SDLK_KP_DECIMAL, Key_NumDecimal },
	{ SDLK_KP_DIVIDE, Key_NumDivide },
	{ SDLK_KP_MULTIPLY, Key_NumMultiply },
	{ SDLK_KP_MINUS, Key_NumSubtract },
	{ SDLK_KP_PLUS, Key_NumAdd },
	{ SDLK_KP_ENTER, Key_NumEnter },
	{ SDLK_KP_EQUALS, Key_NumEqual },
	{ SDLK_LSHIFT, Key_LShift },
	{ SDLK_LCTRL, Key_LCtrl },
	{ SDLK_LALT, Key_LAlt },
	{ SDLK_RSHIFT, Key_RShift },
	{ SDLK_RCTRL, Key_RCtrl },
	{ SDLK_RALT, Key_RAlt },
	{ SDLK_MENU, Key_Menu }
};

void fromSdl(const uint16_t& from, uint8_t* to) {
	if (from & KMOD_CTRL)
		*to |= Mod_Ctrl;
	if (from & KMOD_SHIFT)
		*to |= Mod_Shift;
	if (from & KMOD_ALT)
		*to |= Mod_Alt;
	if (from & KMOD_CAPS)
		*to |= Mod_Caps;
}

void Window::_recreateWindow(entityx::EventManager & events){
	if (_window)
		_closeWindow(events);

	_window = SDL_CreateWindow(
		_windowInfo.title, 
		SDL_WINDOWPOS_CENTERED_DISPLAY(_windowInfo.monitor), 
		SDL_WINDOWPOS_CENTERED_DISPLAY(_windowInfo.monitor),
		_windowInfo.size.x, 
		_windowInfo.size.y, 
		_windowInfo.flags
	);

	
	if (!_window) {
		std::cerr << "Window SDL_CreateWindow: " << SDL_GetError() << std::endl;
		return;
	}
	
	assert(_context); // Context already created once in ctor
	SDL_GL_MakeCurrent(_window, _context);

	SDL_SetRelativeMouseMode((SDL_bool)_windowInfo.lockedCursor);

	// Emit open events
	events.emit<WindowSizeEvent>(WindowSizeEvent{ _windowInfo.size });
	events.emit<FramebufferSizeEvent>(FramebufferSizeEvent{ _windowInfo.size });
	events.emit<WindowOpenEvent>(WindowOpenEvent{ true });
}

void Window::_closeWindow(entityx::EventManager & events){
	if (!_window)
		return;

	SDL_DestroyWindow(_window);
	_window = nullptr;

	// Emit close event
	events.emit<WindowOpenEvent>(WindowOpenEvent{ false });
}

void Window::_pumpWindowEvents(entityx::EventManager & events){
	// Pump SDL events
	SDL_Event e;
	decltype(sdlKeymap)::const_iterator i;
	uint8_t mod;

	while (SDL_PollEvent(&e)) {
		switch (e.type) {

		case SDL_QUIT:
			closeWindow();
			continue;

		case SDL_WINDOWEVENT:
			switch (e.window.event) {

			case SDL_WINDOWEVENT_RESIZED:
				events.emit<WindowSizeEvent>(WindowSizeEvent{ glm::uvec2(e.window.data1, e.window.data2) });
				events.emit<FramebufferSizeEvent>(FramebufferSizeEvent{ glm::uvec2(e.window.data1, e.window.data2) });
				continue;

			case SDL_WINDOWEVENT_ENTER:
				events.emit<CursorEnterEvent>(CursorEnterEvent{ true });
				continue;

			case SDL_WINDOWEVENT_LEAVE:
				events.emit<CursorEnterEvent>(CursorEnterEvent{ false });
				continue;

			case SDL_WINDOWEVENT_FOCUS_LOST:
				events.emit<WindowFocusEvent>(WindowFocusEvent{ false });
				continue;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
				events.emit<WindowFocusEvent>(WindowFocusEvent{ true });
				continue;
			}
			continue;

		case SDL_KEYDOWN:
			i = sdlKeymap.find(e.key.keysym.sym);
			fromSdl(e.key.keysym.mod, &mod);

			if (i != sdlKeymap.end())
				events.emit<KeyInputEvent>(KeyInputEvent{ i->second, Action::Press, mod });
			else
				events.emit<KeyInputEvent>(KeyInputEvent{ Key_Unknown, Action::Press, mod });
			continue;

		case SDL_KEYUP:
			i = sdlKeymap.find(e.key.keysym.sym);
			fromSdl(e.key.keysym.mod, &mod);

			if (i != sdlKeymap.end())
				events.emit<KeyInputEvent>(KeyInputEvent{ i->second, Action::Release, mod });
			else
				events.emit<KeyInputEvent>(KeyInputEvent{ Key_Unknown, Action::Release, mod });
			continue;

		case SDL_MOUSEBUTTONDOWN:
			fromSdl(SDL_GetModState(), &mod);

			events.emit<MousePressEvent>(MousePressEvent{ e.button.button, Action::Press, mod });
			continue;

		case SDL_MOUSEBUTTONUP:
			fromSdl(SDL_GetModState(), &mod);

			events.emit<MousePressEvent>(MousePressEvent{ e.button.button, Action::Release, mod });
			continue;

		case SDL_MOUSEMOTION:
			events.emit<CursorPositionEvent>(CursorPositionEvent{ glm::dvec2(e.motion.x, e.motion.y), glm::dvec2(e.motion.xrel, e.motion.yrel) });
			continue;

		case SDL_MOUSEWHEEL:
			events.emit<ScrollWheelEvent>(ScrollWheelEvent{ glm::dvec2(e.wheel.x, e.wheel.y) });
			continue;
		}
	}
}

Window::Window(const ConstructorInfo& constructorInfo) :
		_constructorInfo(constructorInfo), 
		_windowInfo(_constructorInfo.defaultWindow) {

	if (SDL_Init(SDL_INIT_VIDEO)) {
		std::cerr << "Window SDL_Init:" << SDL_GetError() << std::endl;
		return;
	}

	int error = 0;

	error |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, _constructorInfo.contextVersionMajor);
	error |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, _constructorInfo.contextVersionMinor);

	if (_constructorInfo.coreContex)
		error |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	if (_constructorInfo.debugContext)
		error |= SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	if (error == -1) {
		std::cerr << "Window SDL_GL_SetAttribute: " << SDL_GetError() << std::endl;
		return;
	}

	// Create temporary window to hold OpenGL context (gets destroyed and replaced on first update)
	_window = SDL_CreateWindow("", 0, 0, 0, 0, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

	if (!_window) {
		std::cerr << "Window SDL_CreateWindow: " << SDL_GetError() << std::endl;
		return;
	}

	_context = SDL_GL_CreateContext(_window);

	if (!_context) {
		std::cerr << "Window SDL_GL_CreateContext: " << SDL_GetError() << std::endl;
		return;
	}

	gladLoadGLLoader(SDL_GL_GetProcAddress);

	SDL_GL_MakeCurrent(_window, _context);
}

Window::~Window(){
	if (_context)
		SDL_GL_DeleteContext(_context);

	if (_window)
		SDL_DestroyWindow(_window);

	SDL_Quit();
}

void Window::update(entityx::EntityManager& entities, entityx::EventManager& events, double dt){
	if (!_context)
		return;

	// Check if window needs to be recreated or closed
	if (_open && !_window)
		_recreateWindow(events);
	else if (!_open && _window)
		_closeWindow(events);

	if (_window)
		SDL_GL_SwapWindow(_window);

	// Pump events
	_pumpWindowEvents(events);

	// Destroy initial context window (after events)
	if (_contextWindow) {
		SDL_DestroyWindow(_window);
		_window = nullptr;

		_recreateWindow(events);
		_contextWindow = false;
	}
}

void Window::openWindow(const WindowInfo& windowInfo){
	_windowInfo = windowInfo;
	_open = true;
}

void Window::openWindow(){
	_open = true;
}

void Window::closeWindow(){
	_open = false;
}

void Window::lockCursor(bool lock){
	if (_windowInfo.lockedCursor != lock) {
		_windowInfo.lockedCursor = lock;
		SDL_SetRelativeMouseMode((SDL_bool)lock);
	}
}

bool Window::isOpen() const{
	return _open;
}

Window::WindowInfo Window::windowInfo() const{
	return _windowInfo;
}
