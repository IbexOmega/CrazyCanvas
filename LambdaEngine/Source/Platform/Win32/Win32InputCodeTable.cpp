#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Platform/Win32/Win32InputCodeTable.h"
#include "Platform/Win32/Windows.h"

namespace LambdaEngine
{
	EKey 			Win32InputCodeTable::s_KeyCodeTable[EKey::KEY_COUNT];

	bool Win32InputCodeTable::Init()
	{
		ZERO_MEMORY(s_KeyCodeTable, EKey::KEY_COUNT * sizeof(int32));
		s_KeyCodeTable[VK_SPACE]			= KEY_SPACE;
		//s_KeyCodeTable[0xDE]				= KEY_APOSTROPHE;
		s_KeyCodeTable[VK_OEM_COMMA]		= KEY_COMMA;
		s_KeyCodeTable[VK_OEM_MINUS]		= KEY_MINUS;
		s_KeyCodeTable[VK_OEM_PERIOD]		= KEY_PERIOD;
		//s_KeyCodeTable[KEY_SLASH]			= KEY_SLASH; 
		s_KeyCodeTable[0x30]				= KEY_0;
		s_KeyCodeTable[0x31]				= KEY_1;
		s_KeyCodeTable[0x32]				= KEY_2; 
		s_KeyCodeTable[0x33]				= KEY_3; 
		s_KeyCodeTable[0x34]				= KEY_4; 
		s_KeyCodeTable[0x35]				= KEY_5; 
		s_KeyCodeTable[0x36]				= KEY_6; 
		s_KeyCodeTable[0x37]				= KEY_7; 
		s_KeyCodeTable[0x38]				= KEY_8; 
		s_KeyCodeTable[0x39]				= KEY_9; 
		//s_KeyCodeTable[KEY_SEMICOLON]		= KEY_SEMICOLON; 
		//s_KeyCodeTable[KEY_EQUAL]			= KEY_EQUAL; 
		s_KeyCodeTable[0x41]				= KEY_A;
		s_KeyCodeTable[0x42]				= KEY_B; 
		s_KeyCodeTable[0x43]				= KEY_C; 
		s_KeyCodeTable[0x44]				= KEY_D; 
		s_KeyCodeTable[0x45]				= KEY_E; 
		s_KeyCodeTable[0x46]				= KEY_F; 
		s_KeyCodeTable[0x47]				= KEY_G; 
		s_KeyCodeTable[0x48]				= KEY_H; 
		s_KeyCodeTable[0x49]				= KEY_I; 
		s_KeyCodeTable[0x4A]				= KEY_J; 
		s_KeyCodeTable[0x4B]				= KEY_K; 
		s_KeyCodeTable[0x4C]				= KEY_L; 
		s_KeyCodeTable[0x4D]				= KEY_M; 
		s_KeyCodeTable[0x4E]				= KEY_N; 
		s_KeyCodeTable[0x4F]				= KEY_O; 
		s_KeyCodeTable[0x50]				= KEY_P; 
		s_KeyCodeTable[0x51]				= KEY_Q; 
		s_KeyCodeTable[0x52]				= KEY_R; 
		s_KeyCodeTable[0x53]				= KEY_S; 
		s_KeyCodeTable[0x54]				= KEY_T; 
		s_KeyCodeTable[0x55]				= KEY_U; 
		s_KeyCodeTable[0x56]				= KEY_V; 
		s_KeyCodeTable[0x57]				= KEY_W; 
		s_KeyCodeTable[0x58]				= KEY_X; 
		s_KeyCodeTable[0x59]				= KEY_Y; 
		s_KeyCodeTable[0x5A]				= KEY_Z; 
		//s_KeyCodeTable[KEY_LEFT_BRACKET]	= KEY_LEFT_BRACKET; 
		//s_KeyCodeTable[KEY_BACKSLASH]		= KEY_BACKSLASH; 
		//s_KeyCodeTable[KEY_RIGHT_BRACKET]	= KEY_RIGHT_BRACKET; 
		//s_KeyCodeTable[KEY_GRAVE_ACCENT]	= KEY_GRAVE_ACCENT; 
		//s_KeyCodeTable[KEY_WORLD_1]		= KEY_WORLD_1; 
		//s_KeyCodeTable[KEY_WORLD_2]		= KEY_WORLD_2; 
		s_KeyCodeTable[VK_ESCAPE]			= KEY_ESCAPE;
		s_KeyCodeTable[VK_RETURN]			= KEY_ENTER;
		s_KeyCodeTable[VK_TAB]				= KEY_TAB;
		s_KeyCodeTable[VK_BACK]				= KEY_BACKSPACE;
		s_KeyCodeTable[VK_INSERT]			= KEY_INSERT;
		s_KeyCodeTable[VK_DELETE]			= KEY_DELETE;
		s_KeyCodeTable[VK_RIGHT]			= KEY_RIGHT;
		s_KeyCodeTable[VK_LEFT]				= KEY_LEFT;
		s_KeyCodeTable[VK_DOWN]				= KEY_DOWN;
		s_KeyCodeTable[VK_UP]				= KEY_UP;
		s_KeyCodeTable[VK_PRIOR]			= KEY_PAGE_UP;
		s_KeyCodeTable[VK_NEXT]				= KEY_PAGE_DOWN;
		s_KeyCodeTable[VK_HOME]				= KEY_HOME;
		s_KeyCodeTable[VK_END]				= KEY_END;
		s_KeyCodeTable[VK_CAPITAL]			= KEY_CAPS_LOCK;
		s_KeyCodeTable[VK_SCROLL]			= KEY_SCROLL_LOCK;
		s_KeyCodeTable[VK_NUMLOCK]			= KEY_NUM_LOCK;
		s_KeyCodeTable[VK_SNAPSHOT]			= KEY_PRINT_SCREEN;
		s_KeyCodeTable[VK_PAUSE]			= KEY_PAUSE;
		s_KeyCodeTable[VK_F1]				= KEY_F1;
		s_KeyCodeTable[VK_F2]				= KEY_F2;
		s_KeyCodeTable[VK_F3]				= KEY_F3;
		s_KeyCodeTable[VK_F4]				= KEY_F4;
		s_KeyCodeTable[VK_F5]				= KEY_F5;
		s_KeyCodeTable[VK_F6]				= KEY_F6;
		s_KeyCodeTable[VK_F7]				= KEY_F7;
		s_KeyCodeTable[VK_F8]				= KEY_F8;
		s_KeyCodeTable[VK_F9]				= KEY_F9;
		s_KeyCodeTable[VK_F10]				= KEY_F10;
		s_KeyCodeTable[VK_F11]				= KEY_F11;
		s_KeyCodeTable[VK_F12]				= KEY_F12;
		s_KeyCodeTable[VK_F13]				= KEY_F13;
		s_KeyCodeTable[VK_F14]				= KEY_F14;
		s_KeyCodeTable[VK_F15]				= KEY_F15;
		s_KeyCodeTable[VK_F16]				= KEY_F16;
		s_KeyCodeTable[VK_F17]				= KEY_F17;
		s_KeyCodeTable[VK_F18]				= KEY_F18; 
		s_KeyCodeTable[VK_F19]				= KEY_F19; 
		s_KeyCodeTable[VK_F20]				= KEY_F20; 
		s_KeyCodeTable[VK_F21]				= KEY_F21; 
		s_KeyCodeTable[VK_F22]				= KEY_F22; 
		s_KeyCodeTable[VK_F23]				= KEY_F23; 
		s_KeyCodeTable[VK_F24]				= KEY_F24; 
		s_KeyCodeTable[0x88]				= KEY_F25; 
		s_KeyCodeTable[VK_NUMPAD0]			= KEY_KP_0;
		s_KeyCodeTable[VK_NUMPAD1]			= KEY_KP_1;
		s_KeyCodeTable[VK_NUMPAD2]			= KEY_KP_2;
		s_KeyCodeTable[VK_NUMPAD3]			= KEY_KP_3;
		s_KeyCodeTable[VK_NUMPAD4]			= KEY_KP_4; 
		s_KeyCodeTable[VK_NUMPAD5]			= KEY_KP_5; 
		s_KeyCodeTable[VK_NUMPAD6]			= KEY_KP_6; 
		s_KeyCodeTable[VK_NUMPAD7]			= KEY_KP_7; 
		s_KeyCodeTable[VK_NUMPAD8]			= KEY_KP_8; 
		s_KeyCodeTable[VK_NUMPAD9]			= KEY_KP_9; 
		s_KeyCodeTable[VK_DECIMAL]			= KEY_KP_DECIMAL;
		s_KeyCodeTable[VK_DIVIDE]			= KEY_KP_DIVIDE;
		s_KeyCodeTable[VK_MULTIPLY]			= KEY_KP_MULTIPLY;
		s_KeyCodeTable[VK_SUBTRACT]			= KEY_KP_SUBTRACT;
		s_KeyCodeTable[VK_ADD]				= KEY_KP_ADD;
		//s_KeyCodeTable[KEY_KP_ENTER]		= KEY_KP_ENTER; 
		//s_KeyCodeTable[KEY_KP_EQUAL]		= KEY_KP_EQUAL; 
		s_KeyCodeTable[VK_SHIFT]			= KEY_LEFT_SHIFT;
		s_KeyCodeTable[VK_LSHIFT]			= KEY_LEFT_SHIFT;
		s_KeyCodeTable[VK_RSHIFT]			= KEY_RIGHT_SHIFT;

		s_KeyCodeTable[VK_CONTROL]			= KEY_LEFT_CONTROL;
		s_KeyCodeTable[VK_LCONTROL]			= KEY_LEFT_CONTROL;
		s_KeyCodeTable[VK_RCONTROL]			= KEY_RIGHT_CONTROL; 

		s_KeyCodeTable[VK_MENU]				= KEY_LEFT_ALT;
		s_KeyCodeTable[VK_LMENU]			= KEY_LEFT_ALT;
		s_KeyCodeTable[VK_RMENU]			= KEY_RIGHT_ALT;

		s_KeyCodeTable[VK_LWIN]				= KEY_LEFT_SUPER;
		s_KeyCodeTable[VK_RWIN]				= KEY_RIGHT_SUPER; 
		//s_KeyCodeTable[KEY_MENU]			= KEY_MENU; 

		return true;
	}

	EKey Win32InputCodeTable::GetKey(int32 keyCode)
	{
		return s_KeyCodeTable[keyCode];
	}
}


#endif