#pragma once

#include <cstdint>

#include <glm\vec2.hpp>

enum Modifier : uint8_t {
	Mod_None = 0,
	Mod_Shift = 1,
	Mod_Ctrl = 2,
	Mod_Alt = 4,
	Mod_Caps = 8,
};

enum Action {
	Release,
	Press,
};

enum Key {
	Key_Unknown,
	Key_Space,
	Key_Quote,
	Key_Comma,
	Key_Minus,
	Key_Period,
	Key_Slash,
	Key_0,
	Key_1,
	Key_2,
	Key_3,
	Key_4,
	Key_5,
	Key_6,
	Key_7,
	Key_8,
	Key_9,
	Key_Semicolon,
	Key_Equal,
	Key_A,
	Key_B,
	Key_C,
	Key_D,
	Key_E,
	Key_F,
	Key_G,
	Key_H,
	Key_I,
	Key_J,
	Key_K,
	Key_L,
	Key_M,
	Key_N,
	Key_O,
	Key_P,
	Key_Q,
	Key_R,
	Key_S,
	Key_T,
	Key_U,
	Key_V,
	Key_W,
	Key_X,
	Key_Y,
	Key_Z,
	Key_LBracket,
	Key_Backslash,
	Key_RBracket,
	Key_GraveAccent,
	Key_Escape,
	Key_Enter,
	Key_Tab,
	Key_Backspace,
	Key_Insert,
	Key_Delete,
	Key_Right,
	Key_Left,
	Key_Down,
	Key_Up,
	Key_PageUp,
	Key_PageDown,
	Key_Home,
	Key_End,
	Key_CapsLock,
	Key_ScrollLock,
	Key_NumLock,
	Key_PrintScreen,
	Key_Pause,
	Key_F1,
	Key_F2,
	Key_F3,
	Key_F4,
	Key_F5,
	Key_F6,
	Key_F7,
	Key_F8,
	Key_F9,
	Key_F10,
	Key_F11,
	Key_F12,
	Key_F13,
	Key_F14,
	Key_F15,
	Key_F16,
	Key_F17,
	Key_F18,
	Key_F19,
	Key_F20,
	Key_F21,
	Key_F22,
	Key_F23,
	Key_F24,
	Key_Num0,
	Key_Num1,
	Key_Num2,
	Key_Num3,
	Key_Num4,
	Key_Num5,
	Key_Num6,
	Key_Num7,
	Key_Num8,
	Key_Num9,
	Key_NumDecimal,
	Key_NumDivide,
	Key_NumMultiply,
	Key_NumSubtract,
	Key_NumAdd,
	Key_NumEnter,
	Key_NumEqual,
	Key_LShift,
	Key_LCtrl,
	Key_LAlt,
	Key_RShift,
	Key_RCtrl,
	Key_RAlt,
	Key_Menu
};

struct ScrollWheelEvent {
	glm::dvec2 offset;
};

struct MousePressEvent {
	uint32_t button;
	Action action;
	uint8_t mods;
};

struct TextInputEvent {
	uint32_t utf32;
	uint8_t mods;
};

struct KeyInputEvent {
	uint32_t key;
	Action action;
	uint8_t mods;
};

struct FileDropEvent {
	std::vector<std::string> paths;
};

struct CursorEnterEvent {
	bool entered;
};

struct CursorPositionEvent {
	glm::dvec2 position;
	glm::dvec2 relative;
};

struct WindowPositionEvent {
	glm::uvec2 position;
};

struct FramebufferSizeEvent {
	glm::uvec2 size;
};

struct WindowSizeEvent {
	glm::uvec2 size;
};

struct WindowOpenEvent {
	bool opened;
};

struct WindowFocusEvent {
	bool focused;
};