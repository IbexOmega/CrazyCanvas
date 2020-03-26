#ifdef LAMBDA_PLATFORM_WINDOWS

#include "Platform/Win32/Win32InputCodeTable.h"

namespace LambdaEngine
{
	EKey 			Win32InputCodeTable::s_KeyCodeTable[EKey::KEY_COUNT];
	EMouseButton 	Win32InputCodeTable::s_MouseButtonCodeTable[EMouseButton::MOUSE_BUTTON_COUNT];

	bool Win32InputCodeTable::Init()
	{
		ZERO_MEMORY(s_KeyCodeTable, EKey::KEY_COUNT * sizeof(int32));

		s_KeyCodeTable[KEY_SPACE]			= KEY_SPACE; 
		s_KeyCodeTable[KEY_APOSTROPHE]		= KEY_APOSTROPHE; 
		s_KeyCodeTable[KEY_COMMA]			= KEY_COMMA; 
		s_KeyCodeTable[KEY_MINUS]			= KEY_MINUS; 
		s_KeyCodeTable[KEY_PERIOD]			= KEY_PERIOD; 
		s_KeyCodeTable[KEY_SLASH]			= KEY_SLASH; 
		s_KeyCodeTable[KEY_0]				= KEY_0; 
		s_KeyCodeTable[KEY_1]				= KEY_1; 
		s_KeyCodeTable[KEY_2]				= KEY_2; 
		s_KeyCodeTable[KEY_3]				= KEY_3; 
		s_KeyCodeTable[KEY_4]				= KEY_4; 
		s_KeyCodeTable[KEY_5]				= KEY_5; 
		s_KeyCodeTable[KEY_6]				= KEY_6; 
		s_KeyCodeTable[KEY_7]				= KEY_7; 
		s_KeyCodeTable[KEY_8]				= KEY_8; 
		s_KeyCodeTable[KEY_9]				= KEY_9; 
		s_KeyCodeTable[KEY_SEMICOLON]		= KEY_SEMICOLON; 
		s_KeyCodeTable[KEY_EQUAL]			= KEY_EQUAL; 
		s_KeyCodeTable[KEY_A]				= KEY_A; 
		s_KeyCodeTable[KEY_B]				= KEY_B; 
		s_KeyCodeTable[KEY_C]				= KEY_C; 
		s_KeyCodeTable[KEY_D]				= KEY_D; 
		s_KeyCodeTable[KEY_E]				= KEY_E; 
		s_KeyCodeTable[KEY_F]				= KEY_F; 
		s_KeyCodeTable[KEY_G]				= KEY_G; 
		s_KeyCodeTable[KEY_H]				= KEY_H; 
		s_KeyCodeTable[KEY_I]				= KEY_I; 
		s_KeyCodeTable[KEY_J]				= KEY_J; 
		s_KeyCodeTable[KEY_K]				= KEY_K; 
		s_KeyCodeTable[KEY_L]				= KEY_L; 
		s_KeyCodeTable[KEY_M]				= KEY_M; 
		s_KeyCodeTable[KEY_N]				= KEY_N; 
		s_KeyCodeTable[KEY_O]				= KEY_O; 
		s_KeyCodeTable[KEY_P]				= KEY_P; 
		s_KeyCodeTable[KEY_Q]				= KEY_Q; 
		s_KeyCodeTable[KEY_R]				= KEY_R; 
		s_KeyCodeTable[KEY_S]				= KEY_S; 
		s_KeyCodeTable[KEY_T]				= KEY_T; 
		s_KeyCodeTable[KEY_U]				= KEY_U; 
		s_KeyCodeTable[KEY_V]				= KEY_V; 
		s_KeyCodeTable[KEY_W]				= KEY_W; 
		s_KeyCodeTable[KEY_X]				= KEY_X; 
		s_KeyCodeTable[KEY_Y]				= KEY_Y; 
		s_KeyCodeTable[KEY_Z]				= KEY_Z; 
		s_KeyCodeTable[KEY_LEFT_BRACKET]	= KEY_LEFT_BRACKET; 
		s_KeyCodeTable[KEY_BACKSLASH]		= KEY_BACKSLASH; 
		s_KeyCodeTable[KEY_RIGHT_BRACKET]	= KEY_RIGHT_BRACKET; 
		s_KeyCodeTable[KEY_GRAVE_ACCENT]	= KEY_GRAVE_ACCENT; 
		s_KeyCodeTable[KEY_WORLD_1]			= KEY_WORLD_1; 
		s_KeyCodeTable[KEY_WORLD_2]			= KEY_WORLD_2; 
		s_KeyCodeTable[KEY_ESCAPE]			= KEY_ESCAPE; 
		s_KeyCodeTable[KEY_ENTER]			= KEY_ENTER; 
		s_KeyCodeTable[KEY_TAB]				= KEY_TAB; 
		s_KeyCodeTable[KEY_BACKSPACE]		= KEY_BACKSPACE; 
		s_KeyCodeTable[KEY_INSERT]			= KEY_INSERT; 
		s_KeyCodeTable[KEY_DELETE]			= KEY_DELETE; 
		s_KeyCodeTable[KEY_RIGHT]			= KEY_RIGHT; 
		s_KeyCodeTable[KEY_LEFT]			= KEY_LEFT; 
		s_KeyCodeTable[KEY_DOWN]			= KEY_DOWN; 
		s_KeyCodeTable[KEY_UP]				= KEY_UP; 
		s_KeyCodeTable[KEY_PAGE_UP]			= KEY_PAGE_UP; 
		s_KeyCodeTable[KEY_PAGE_DOWN]		= KEY_PAGE_DOWN; 
		s_KeyCodeTable[KEY_HOME]			= KEY_HOME; 
		s_KeyCodeTable[KEY_END]				= KEY_END; 
		s_KeyCodeTable[KEY_CAPS_LOCK]		= KEY_CAPS_LOCK; 
		s_KeyCodeTable[KEY_SCROLL_LOCK]		= KEY_SCROLL_LOCK; 
		s_KeyCodeTable[KEY_NUM_LOCK]		= KEY_NUM_LOCK; 
		s_KeyCodeTable[KEY_PRINT_SCREEN]	= KEY_PRINT_SCREEN; 
		s_KeyCodeTable[KEY_PAUSE]			= KEY_PAUSE; 
		s_KeyCodeTable[KEY_F1]				= KEY_F1; 
		s_KeyCodeTable[KEY_F2]				= KEY_F2; 
		s_KeyCodeTable[KEY_F3]				= KEY_F3; 
		s_KeyCodeTable[KEY_F4]				= KEY_F4; 
		s_KeyCodeTable[KEY_F5]				= KEY_F5; 
		s_KeyCodeTable[KEY_F6]				= KEY_F6; 
		s_KeyCodeTable[KEY_F7]				= KEY_F7; 
		s_KeyCodeTable[KEY_F8]				= KEY_F8; 
		s_KeyCodeTable[KEY_F9]				= KEY_F9; 
		s_KeyCodeTable[KEY_F10]				= KEY_F10; 
		s_KeyCodeTable[KEY_F11]				= KEY_F11; 
		s_KeyCodeTable[KEY_F12]				= KEY_F12; 
		s_KeyCodeTable[KEY_F13]				= KEY_F13; 
		s_KeyCodeTable[KEY_F14]				= KEY_F14; 
		s_KeyCodeTable[KEY_F15]				= KEY_F15; 
		s_KeyCodeTable[KEY_F16]				= KEY_F16; 
		s_KeyCodeTable[KEY_F17]				= KEY_F17; 
		s_KeyCodeTable[KEY_F18]				= KEY_F18; 
		s_KeyCodeTable[KEY_F19]				= KEY_F19; 
		s_KeyCodeTable[KEY_F20]				= KEY_F20; 
		s_KeyCodeTable[KEY_F21]				= KEY_F21; 
		s_KeyCodeTable[KEY_F22]				= KEY_F22; 
		s_KeyCodeTable[KEY_F23]				= KEY_F23; 
		s_KeyCodeTable[KEY_F24]				= KEY_F24; 
		s_KeyCodeTable[KEY_F25]				= KEY_F25; 
		s_KeyCodeTable[KEY_KP_0]			= KEY_KP_0; 
		s_KeyCodeTable[KEY_KP_1]			= KEY_KP_1; 
		s_KeyCodeTable[KEY_KP_2]			= KEY_KP_2; 
		s_KeyCodeTable[KEY_KP_3]			= KEY_KP_3; 
		s_KeyCodeTable[KEY_KP_4]			= KEY_KP_4; 
		s_KeyCodeTable[KEY_KP_5]			= KEY_KP_5; 
		s_KeyCodeTable[KEY_KP_6]			= KEY_KP_6; 
		s_KeyCodeTable[KEY_KP_7]			= KEY_KP_7; 
		s_KeyCodeTable[KEY_KP_8]			= KEY_KP_8; 
		s_KeyCodeTable[KEY_KP_9]			= KEY_KP_9; 
		s_KeyCodeTable[KEY_KP_DECIMAL]		= KEY_KP_DECIMAL; 
		s_KeyCodeTable[KEY_KP_DIVIDE]		= KEY_KP_DIVIDE; 
		s_KeyCodeTable[KEY_KP_MULTIPLY]		= KEY_KP_MULTIPLY; 
		s_KeyCodeTable[KEY_KP_SUBTRACT]		= KEY_KP_SUBTRACT; 
		s_KeyCodeTable[KEY_KP_ADD]			= KEY_KP_ADD; 
		s_KeyCodeTable[KEY_KP_ENTER]		= KEY_KP_ENTER; 
		s_KeyCodeTable[KEY_KP_EQUAL]		= KEY_KP_EQUAL; 
		s_KeyCodeTable[KEY_LEFT_SHIFT]		= KEY_LEFT_SHIFT; 
		s_KeyCodeTable[KEY_LEFT_CONTROL]	= KEY_LEFT_CONTROL; 
		s_KeyCodeTable[KEY_LEFT_ALT]		= KEY_LEFT_ALT; 
		s_KeyCodeTable[KEY_LEFT_SUPER]		= KEY_LEFT_SUPER; 
		s_KeyCodeTable[KEY_RIGHT_SHIFT]		= KEY_RIGHT_SHIFT; 
		s_KeyCodeTable[KEY_RIGHT_CONTROL]	= KEY_RIGHT_CONTROL; 
		s_KeyCodeTable[KEY_RIGHT_ALT]		= KEY_RIGHT_ALT; 
		s_KeyCodeTable[KEY_RIGHT_SUPER]		= KEY_RIGHT_SUPER; 
		s_KeyCodeTable[KEY_MENU]			= KEY_MENU; 

		ZERO_MEMORY(s_MouseButtonCodeTable, EMouseButton::MOUSE_BUTTON_COUNT * sizeof(int32));

		s_MouseButtonCodeTable[MOUSE_BUTTON_LEFT]		= MOUSE_BUTTON_LEFT;
		s_MouseButtonCodeTable[MOUSE_BUTTON_MIDDLE]		= MOUSE_BUTTON_MIDDLE;
		s_MouseButtonCodeTable[MOUSE_BUTTON_RIGHT]		= MOUSE_BUTTON_RIGHT;
		s_MouseButtonCodeTable[MOUSE_BUTTON_BACK]		= MOUSE_BUTTON_BACK;
		s_MouseButtonCodeTable[MOUSE_BUTTON_FORWARD]	= MOUSE_BUTTON_FORWARD;

		return true;
	}

	EKey Win32InputCodeTable::GetKey(int32 keyCode)
	{
		return s_KeyCodeTable[keyCode];
	}

	EMouseButton Win32InputCodeTable::GetMouseButton(int32 mouseButtonCode)
	{
		return s_MouseButtonCodeTable[mouseButtonCode];
	}
}


#endif