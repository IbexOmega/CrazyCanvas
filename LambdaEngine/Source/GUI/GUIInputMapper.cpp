#include "GUI/GUIInputMapper.h"

namespace LambdaEngine
{
	Noesis::Key GUIInputMapper::s_KeyMapping[EKey::KEY_COUNT];
	Noesis::MouseButton GUIInputMapper::s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_COUNT + 1];

	bool GUIInputMapper::Init()
	{
		s_KeyMapping[KEY_0]		= Noesis::Key::Key_D0;
		s_KeyMapping[KEY_1]		= Noesis::Key::Key_D1;
		s_KeyMapping[KEY_2]		= Noesis::Key::Key_D2;
		s_KeyMapping[KEY_3]		= Noesis::Key::Key_D3;
		s_KeyMapping[KEY_4]		= Noesis::Key::Key_D4;
		s_KeyMapping[KEY_5]		= Noesis::Key::Key_D5;
		s_KeyMapping[KEY_6]		= Noesis::Key::Key_D6;
		s_KeyMapping[KEY_7]		= Noesis::Key::Key_D7;
		s_KeyMapping[KEY_8]		= Noesis::Key::Key_D8;
		s_KeyMapping[KEY_9]		= Noesis::Key::Key_D9;

		s_KeyMapping[KEY_A]		= Noesis::Key::Key_A;
		s_KeyMapping[KEY_B]		= Noesis::Key::Key_B;
		s_KeyMapping[KEY_C]		= Noesis::Key::Key_C;
		s_KeyMapping[KEY_D]		= Noesis::Key::Key_D;
		s_KeyMapping[KEY_E]		= Noesis::Key::Key_E;
		s_KeyMapping[KEY_F]		= Noesis::Key::Key_F;
		s_KeyMapping[KEY_G]		= Noesis::Key::Key_G;
		s_KeyMapping[KEY_H]		= Noesis::Key::Key_H;
		s_KeyMapping[KEY_I]		= Noesis::Key::Key_I;
		s_KeyMapping[KEY_J]		= Noesis::Key::Key_J;
		s_KeyMapping[KEY_K]		= Noesis::Key::Key_K;
		s_KeyMapping[KEY_L]		= Noesis::Key::Key_L;
		s_KeyMapping[KEY_M]		= Noesis::Key::Key_M;
		s_KeyMapping[KEY_N]		= Noesis::Key::Key_N;
		s_KeyMapping[KEY_O]		= Noesis::Key::Key_O;
		s_KeyMapping[KEY_P]		= Noesis::Key::Key_P;
		s_KeyMapping[KEY_Q]		= Noesis::Key::Key_Q;
		s_KeyMapping[KEY_R]		= Noesis::Key::Key_R;
		s_KeyMapping[KEY_S]		= Noesis::Key::Key_S;
		s_KeyMapping[KEY_T]		= Noesis::Key::Key_T;
		s_KeyMapping[KEY_U]		= Noesis::Key::Key_U;
		s_KeyMapping[KEY_V]		= Noesis::Key::Key_V;
		s_KeyMapping[KEY_W]		= Noesis::Key::Key_W;
		s_KeyMapping[KEY_X]		= Noesis::Key::Key_X;
		s_KeyMapping[KEY_Y]		= Noesis::Key::Key_Y;
		s_KeyMapping[KEY_Z]		= Noesis::Key::Key_Z;

		s_KeyMapping[KEY_F1]	= Noesis::Key::Key_F1;
		s_KeyMapping[KEY_F2]	= Noesis::Key::Key_F2;
		s_KeyMapping[KEY_F3]	= Noesis::Key::Key_F3;
		s_KeyMapping[KEY_F4]	= Noesis::Key::Key_F4;
		s_KeyMapping[KEY_F5]	= Noesis::Key::Key_F5;
		s_KeyMapping[KEY_F6]	= Noesis::Key::Key_F6;
		s_KeyMapping[KEY_F7]	= Noesis::Key::Key_F7;
		s_KeyMapping[KEY_F8]	= Noesis::Key::Key_F8;
		s_KeyMapping[KEY_F9]	= Noesis::Key::Key_F9;
		s_KeyMapping[KEY_F10]	= Noesis::Key::Key_F10;
		s_KeyMapping[KEY_F11]	= Noesis::Key::Key_F11;
		s_KeyMapping[KEY_F12]	= Noesis::Key::Key_F12;
		s_KeyMapping[KEY_F13]	= Noesis::Key::Key_F13;
		s_KeyMapping[KEY_F14]	= Noesis::Key::Key_F14;
		s_KeyMapping[KEY_F15]	= Noesis::Key::Key_F15;
		s_KeyMapping[KEY_F16]	= Noesis::Key::Key_F16;
		s_KeyMapping[KEY_F17]	= Noesis::Key::Key_F17;
		s_KeyMapping[KEY_F18]	= Noesis::Key::Key_F18;
		s_KeyMapping[KEY_F19]	= Noesis::Key::Key_F19;
		s_KeyMapping[KEY_F20]	= Noesis::Key::Key_F20;
		s_KeyMapping[KEY_F21]	= Noesis::Key::Key_F21;
		s_KeyMapping[KEY_F22]	= Noesis::Key::Key_F22;
		s_KeyMapping[KEY_F23]	= Noesis::Key::Key_F23;
		s_KeyMapping[KEY_F24]	= Noesis::Key::Key_F24;
		s_KeyMapping[KEY_F25]	= Noesis::Key::Key_VolumeUp; //Hehe, Prank

		s_KeyMapping[KEY_KEYPAD_0]	= Noesis::Key::Key_NumPad0;
		s_KeyMapping[KEY_KEYPAD_1]	= Noesis::Key::Key_NumPad1;
		s_KeyMapping[KEY_KEYPAD_2]	= Noesis::Key::Key_NumPad2;
		s_KeyMapping[KEY_KEYPAD_3]	= Noesis::Key::Key_NumPad3;
		s_KeyMapping[KEY_KEYPAD_4]	= Noesis::Key::Key_NumPad4;
		s_KeyMapping[KEY_KEYPAD_5]	= Noesis::Key::Key_NumPad5;
		s_KeyMapping[KEY_KEYPAD_6]	= Noesis::Key::Key_NumPad6;
		s_KeyMapping[KEY_KEYPAD_7]	= Noesis::Key::Key_NumPad7;
		s_KeyMapping[KEY_KEYPAD_8]	= Noesis::Key::Key_NumPad8;
		s_KeyMapping[KEY_KEYPAD_9]	= Noesis::Key::Key_NumPad9;

		s_KeyMapping[KEY_KEYPAD_DECIMAL]	= Noesis::Key::Key_Decimal;
		s_KeyMapping[KEY_KEYPAD_DIVIDE]		= Noesis::Key::Key_Divide;
		s_KeyMapping[KEY_KEYPAD_MULTIPLY]	= Noesis::Key::Key_Multiply;
		s_KeyMapping[KEY_KEYPAD_SUBTRACT]	= Noesis::Key::Key_Subtract;
		s_KeyMapping[KEY_KEYPAD_ADD]		= Noesis::Key::Key_Add;
		s_KeyMapping[KEY_KEYPAD_ENTER]		= Noesis::Key::Key_Enter;
		s_KeyMapping[KEY_KEYPAD_EQUAL]		= Noesis::Key::Key_VolumeDown; //Hehe, Prank 2

		s_KeyMapping[KEY_LEFT_SHIFT]		= Noesis::Key::Key_LeftShift;
		s_KeyMapping[KEY_LEFT_CONTROL]		= Noesis::Key::Key_LeftCtrl;
		s_KeyMapping[KEY_LEFT_ALT]			= Noesis::Key::Key_LeftAlt;
		s_KeyMapping[KEY_LEFT_SUPER]		= Noesis::Key::Key_LWin;
		s_KeyMapping[KEY_RIGHT_SHIFT]		= Noesis::Key::Key_RightShift;
		s_KeyMapping[KEY_RIGHT_CONTROL]		= Noesis::Key::Key_RightCtrl;
		s_KeyMapping[KEY_RIGHT_ALT]			= Noesis::Key::Key_RightAlt;
		s_KeyMapping[KEY_RIGHT_SUPER]		= Noesis::Key::Key_RWin;

		s_KeyMapping[KEY_MENU]				= Noesis::Key::Key_GamepadMenu;
		s_KeyMapping[KEY_SPACE]				= Noesis::Key::Key_Space;
		//s_KeyMapping[KEY_APOSTROPHE]		= Noesis::Key::; IDK
		s_KeyMapping[KEY_COMMA]				= Noesis::Key::Key_OemComma;
		s_KeyMapping[KEY_MINUS]				= Noesis::Key::Key_OemMinus;
		s_KeyMapping[KEY_PERIOD]			= Noesis::Key::Key_OemPeriod;
		//s_KeyMapping[KEY_SLASH]			= Noesis::Key::Key_Decimal;
		s_KeyMapping[KEY_SEMICOLON]			= Noesis::Key::Key_OemSemicolon;
		//s_KeyMapping[KEY_EQUAL]			= Noesis::Key::Key_Decimal;
		s_KeyMapping[KEY_LEFT_BRACKET]		= Noesis::Key::Key_OemOpenBrackets;
		s_KeyMapping[KEY_BACKSLASH]			= Noesis::Key::Key_OemBackslash;
		s_KeyMapping[KEY_RIGHT_BRACKET]		= Noesis::Key::Key_Decimal;
		s_KeyMapping[KEY_GRAVE_ACCENT]		= Noesis::Key::Key_OemCloseBrackets;
		//s_KeyMapping[KEY_WORLD_1]			= Noesis::Key::Key_Decimal;
		//s_KeyMapping[KEY_WORLD_2]			= Noesis::Key::Key_Decimal;
		s_KeyMapping[KEY_ESCAPE]			= Noesis::Key::Key_Escape;
		s_KeyMapping[KEY_ENTER]				= Noesis::Key::Key_Enter;
		s_KeyMapping[KEY_TAB]				= Noesis::Key::Key_Tab;
		s_KeyMapping[KEY_BACKSPACE]			= Noesis::Key::Key_Back;
		s_KeyMapping[KEY_INSERT]			= Noesis::Key::Key_Insert;
		s_KeyMapping[KEY_DELETE]			= Noesis::Key::Key_Delete;
		s_KeyMapping[KEY_RIGHT]				= Noesis::Key::Key_Right;
		s_KeyMapping[KEY_LEFT]				= Noesis::Key::Key_Left;
		s_KeyMapping[KEY_DOWN]				= Noesis::Key::Key_Down;
		s_KeyMapping[KEY_UP]				= Noesis::Key::Key_Up;
		s_KeyMapping[KEY_PAGE_UP]			= Noesis::Key::Key_PageUp;
		s_KeyMapping[KEY_PAGE_DOWN]			= Noesis::Key::Key_PageDown;
		s_KeyMapping[KEY_HOME]				= Noesis::Key::Key_Home;
		s_KeyMapping[KEY_END]				= Noesis::Key::Key_End;
		s_KeyMapping[KEY_CAPS_LOCK]			= Noesis::Key::Key_CapsLock;
		s_KeyMapping[KEY_SCROLL_LOCK]		= Noesis::Key::Key_Scroll;
		s_KeyMapping[KEY_NUM_LOCK]			= Noesis::Key::Key_NumLock;
		s_KeyMapping[KEY_PRINT_SCREEN]		= Noesis::Key::Key_PrintScreen;
		s_KeyMapping[KEY_PAUSE]				= Noesis::Key::Key_Pause;

		s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_LEFT]		= Noesis::MouseButton::MouseButton_Left;
		s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_RIGHT]		= Noesis::MouseButton::MouseButton_Right;
		s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_MIDDLE]		= Noesis::MouseButton::MouseButton_Middle;
		s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_FORWARD]	= Noesis::MouseButton::MouseButton_XButton1;
		s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_BACK]		= Noesis::MouseButton::MouseButton_XButton2;

		return true;
	}

	Noesis::Key GUIInputMapper::GetKey(EKey key)
	{
		VALIDATE(key < ARR_SIZE(s_KeyMapping));

		return s_KeyMapping[key];
	}

	Noesis::MouseButton GUIInputMapper::GetMouseButton(EMouseButton button)
	{
		VALIDATE(button < ARR_SIZE(s_MouseButtonMapping));

		return s_MouseButtonMapping[button];
	}
}