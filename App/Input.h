#pragma once

#include <stdint.h>
#include <iostream>
#include <map>
#include <vector>
#include <functional>

#include <glm/glm.hpp>

#include "Application.h"
#include <GLFW/glfw3.h>

namespace Celestiq {

	typedef enum class KeyCode : uint16_t
	{
		// From glfw3.h
		Space = 32,
		Apostrophe = 39, /* ' */
		Comma = 44, /* , */
		Minus = 45, /* - */
		Period = 46, /* . */
		Slash = 47, /* / */

		D0 = 48, /* 0 */
		D1 = 49, /* 1 */
		D2 = 50, /* 2 */
		D3 = 51, /* 3 */
		D4 = 52, /* 4 */
		D5 = 53, /* 5 */
		D6 = 54, /* 6 */
		D7 = 55, /* 7 */
		D8 = 56, /* 8 */
		D9 = 57, /* 9 */

		Semicolon = 59, /* ; */
		Equal = 61, /* = */

		A = 65,
		B = 66,
		C = 67,
		D = 68,
		E = 69,
		F = 70,
		G = 71,
		H = 72,
		I = 73,
		J = 74,
		K = 75,
		L = 76,
		M = 77,
		N = 78,
		O = 79,
		P = 80,
		Q = 81,
		R = 82,
		S = 83,
		T = 84,
		U = 85,
		V = 86,
		W = 87,
		X = 88,
		Y = 89,
		Z = 90,

		LeftBracket = 91,  /* [ */
		Backslash = 92,  /* \ */
		RightBracket = 93,  /* ] */
		GraveAccent = 96,  /* ` */

		World1 = 161, /* non-US #1 */
		World2 = 162, /* non-US #2 */

		/* Function keys */
		Escape = 256,
		Enter = 257,
		Tab = 258,
		Backspace = 259,
		Insert = 260,
		Delete = 261,
		Right = 262,
		Left = 263,
		Down = 264,
		Up = 265,
		PageUp = 266,
		PageDown = 267,
		Home = 268,
		End = 269,
		CapsLock = 280,
		ScrollLock = 281,
		NumLock = 282,
		PrintScreen = 283,
		Pause = 284,
		F1 = 290,
		F2 = 291,
		F3 = 292,
		F4 = 293,
		F5 = 294,
		F6 = 295,
		F7 = 296,
		F8 = 297,
		F9 = 298,
		F10 = 299,
		F11 = 300,
		F12 = 301,
		F13 = 302,
		F14 = 303,
		F15 = 304,
		F16 = 305,
		F17 = 306,
		F18 = 307,
		F19 = 308,
		F20 = 309,
		F21 = 310,
		F22 = 311,
		F23 = 312,
		F24 = 313,
		F25 = 314,

		/* Keypad */
		KP0 = 320,
		KP1 = 321,
		KP2 = 322,
		KP3 = 323,
		KP4 = 324,
		KP5 = 325,
		KP6 = 326,
		KP7 = 327,
		KP8 = 328,
		KP9 = 329,
		KPDecimal = 330,
		KPDivide = 331,
		KPMultiply = 332,
		KPSubtract = 333,
		KPAdd = 334,
		KPEnter = 335,
		KPEqual = 336,

		LeftShift = 340,
		LeftControl = 341,
		LeftAlt = 342,
		LeftSuper = 343,
		RightShift = 344,
		RightControl = 345,
		RightAlt = 346,
		RightSuper = 347,
		Menu = 348
	} Key;

	enum class KeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	enum class CursorMode
	{
		Normal = 0,
		Hidden = 1,
		Locked = 2
	};

	typedef enum class MouseButton : uint16_t
	{
		Button0 = 0,
		Button1 = 1,
		Button2 = 2,
		Button3 = 3,
		Button4 = 4,
		Button5 = 5,
		Left = Button0,
		Right = Button1,
		Middle = Button2
	} Button;


	inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
	{
		os << static_cast<int32_t>(keyCode);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, MouseButton button)
	{
		os << static_cast<int32_t>(button);
		return os;
	}

	class Input
	{
	public:
		static Input& getInstance() {
        	static Input instance; 
    	    return instance;
    	}

    	Input(const Input&) = delete;
    	Input& operator=(const Input&) = delete;

		using KeyCallback = std::function<void()>;
		// 注册按键事件回调
    	void registerKeyCallback(KeyCode key, KeyState state, KeyCallback callback) {
    	    keyCallbacks[{key, state}].push_back(callback);
    	}

		// 每帧更新
		void update()
		{
			pull();
			if(IsKeyPressed(KeyCode::Tab)){
				if(cursorMode == CursorMode::Normal){
					SetCursorMode(CursorMode::Locked);
					cursorMode = CursorMode::Locked;
				}else if(cursorMode == CursorMode::Locked){
					SetCursorMode(CursorMode::Normal);
					cursorMode = CursorMode::Normal;
				}
			}
			// 遍历注册的键盘事件
			for (auto& [keyType, callbacks] : keyCallbacks) {
				KeyCode key = keyType.first;
				KeyState type = keyType.second;
	
				bool triggered = false;
				if (type == KeyState::Pressed) {
					triggered = IsKeyPressed(key);
				} else if (type == KeyState::Held) {
					triggered = IsKeyDown(key);
				}
	
				if (triggered) {
					for (auto& cb : callbacks)
						cb();
				}
			}
			previousKeyStates = currentKeyStates;
		}

		CursorMode GetCursorMode() const
		{
			return cursorMode;
		}

		// 检测按下瞬间
		bool IsKeyPressed(KeyCode keycode)
		{
		    bool isCurrentlyDown = currentKeyStates[keycode];
		    bool wasPreviouslyDown = previousKeyStates[keycode];
			
		    return isCurrentlyDown && !wasPreviouslyDown;
		}

		// 检测按下保持
		static bool IsKeyDown(KeyCode keycode)
		{
			GLFWwindow* windowHandle = Application::Get().GetWindowHandle();
			int state = glfwGetKey(windowHandle, (int)keycode);
			return state == GLFW_PRESS || state == GLFW_REPEAT;
		}

		static bool IsMouseButtonDown(MouseButton button)
		{
			GLFWwindow* windowHandle = Application::Get().GetWindowHandle();
			int state = glfwGetMouseButton(windowHandle, (int)button);
			return state == GLFW_PRESS;
		}

		static glm::vec2 GetMousePosition()
		{
			GLFWwindow* windowHandle = Application::Get().GetWindowHandle();
	
			double x, y;
			glfwGetCursorPos(windowHandle, &x, &y);
			return { (float)x, (float)y };
		}

		static void SetCursorMode(CursorMode mode)
		{
			GLFWwindow* windowHandle = Application::Get().GetWindowHandle();
			glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
		}
	private:
		Input() = default; 
		~Input() = default; 
		CursorMode cursorMode = CursorMode::Normal;
		// 记录上一帧的按键状态
		std::unordered_map<KeyCode, bool> previousKeyStates;
		// 记录当前帧的按键状态
		std::unordered_map<KeyCode, bool> currentKeyStates;
		// 查询所有按键状态
		void pull()
		{
			for (KeyCode key = KeyCode::Space; key <= KeyCode::Menu; key = (KeyCode)((int)key + 1)) {
				currentKeyStates[key] = IsKeyDown(key);
			}
		}

		// <(KeyCode, KeyEventType), 回调列表>
		std::map<std::pair<KeyCode, KeyState>, std::vector<KeyCallback>> keyCallbacks;
	};
}
