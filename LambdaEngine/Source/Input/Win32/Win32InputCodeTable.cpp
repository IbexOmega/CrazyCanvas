#ifdef LAMBDA_PLATFORM_WINDOWS
#include "Input/Win32/Win32InputCodeTable.h"

#include "Application/Win32/Windows.h"

namespace LambdaEngine
{
	EKey	Win32InputCodeTable::s_KeyCodeTable[NUM_KEY_CODES];
	uint16	Win32InputCodeTable::s_ScanCodeTable[NUM_KEY_CODES];

	bool Win32InputCodeTable::Init()
	{
		ZERO_MEMORY(s_KeyCodeTable,		sizeof(s_KeyCodeTable));
		ZERO_MEMORY(s_ScanCodeTable,	sizeof(s_ScanCodeTable));

		/* ScanCodes from GLFW - File: win32_init.c */
		s_KeyCodeTable[0x00B]	= EKey::KEY_0;
		s_KeyCodeTable[0x002]	= EKey::KEY_1;
		s_KeyCodeTable[0x003]	= EKey::KEY_2;
		s_KeyCodeTable[0x004]	= EKey::KEY_3;
		s_KeyCodeTable[0x005]	= EKey::KEY_4;
		s_KeyCodeTable[0x006]	= EKey::KEY_5;
		s_KeyCodeTable[0x007]	= EKey::KEY_6;
		s_KeyCodeTable[0x008]	= EKey::KEY_7;
		s_KeyCodeTable[0x009]	= EKey::KEY_8;
		s_KeyCodeTable[0x00A]	= EKey::KEY_9;

		s_KeyCodeTable[0x01E]	= EKey::KEY_A;
		s_KeyCodeTable[0x030]	= EKey::KEY_B;
		s_KeyCodeTable[0x02E]	= EKey::KEY_C;
		s_KeyCodeTable[0x020]	= EKey::KEY_D;
		s_KeyCodeTable[0x012]	= EKey::KEY_E;
		s_KeyCodeTable[0x021]	= EKey::KEY_F;
		s_KeyCodeTable[0x022]	= EKey::KEY_G;
		s_KeyCodeTable[0x023]	= EKey::KEY_H;
		s_KeyCodeTable[0x017]	= EKey::KEY_I;
		s_KeyCodeTable[0x024]	= EKey::KEY_J;
		s_KeyCodeTable[0x025]	= EKey::KEY_K;
		s_KeyCodeTable[0x026]	= EKey::KEY_L;
		s_KeyCodeTable[0x032]	= EKey::KEY_M;
		s_KeyCodeTable[0x031]	= EKey::KEY_N;
		s_KeyCodeTable[0x018]	= EKey::KEY_O;
		s_KeyCodeTable[0x019]	= EKey::KEY_P;
		s_KeyCodeTable[0x010]	= EKey::KEY_Q;
		s_KeyCodeTable[0x013]	= EKey::KEY_R;
		s_KeyCodeTable[0x01F]	= EKey::KEY_S;
		s_KeyCodeTable[0x014]	= EKey::KEY_T;
		s_KeyCodeTable[0x016]	= EKey::KEY_U;
		s_KeyCodeTable[0x02F]	= EKey::KEY_V;
		s_KeyCodeTable[0x011]	= EKey::KEY_W;
		s_KeyCodeTable[0x02D]	= EKey::KEY_X;
		s_KeyCodeTable[0x015]	= EKey::KEY_Y;
		s_KeyCodeTable[0x02C]	= EKey::KEY_Z;

		s_KeyCodeTable[0x03B]	= EKey::KEY_F1;
		s_KeyCodeTable[0x03C]	= EKey::KEY_F2;
		s_KeyCodeTable[0x03D]	= EKey::KEY_F3;
		s_KeyCodeTable[0x03E]	= EKey::KEY_F4;
		s_KeyCodeTable[0x03F]	= EKey::KEY_F5;
		s_KeyCodeTable[0x040]	= EKey::KEY_F6;
		s_KeyCodeTable[0x041]	= EKey::KEY_F7;
		s_KeyCodeTable[0x042]	= EKey::KEY_F8;
		s_KeyCodeTable[0x043]	= EKey::KEY_F9;
		s_KeyCodeTable[0x044]	= EKey::KEY_F10;
		s_KeyCodeTable[0x057]	= EKey::KEY_F11;
		s_KeyCodeTable[0x058]	= EKey::KEY_F12;
		s_KeyCodeTable[0x064]	= EKey::KEY_F13;
		s_KeyCodeTable[0x065]	= EKey::KEY_F14;
		s_KeyCodeTable[0x066]	= EKey::KEY_F15;
		s_KeyCodeTable[0x067]	= EKey::KEY_F16;
		s_KeyCodeTable[0x068]	= EKey::KEY_F17;
		s_KeyCodeTable[0x069]	= EKey::KEY_F18;
		s_KeyCodeTable[0x06A]	= EKey::KEY_F19;
		s_KeyCodeTable[0x06B]	= EKey::KEY_F20;
		s_KeyCodeTable[0x06C]	= EKey::KEY_F21;
		s_KeyCodeTable[0x06D]	= EKey::KEY_F22;
		s_KeyCodeTable[0x06E]	= EKey::KEY_F23;
		s_KeyCodeTable[0x076]	= EKey::KEY_F24;

		s_KeyCodeTable[0x052]	= EKey::KEY_KEYPAD_0;
		s_KeyCodeTable[0x04F]	= EKey::KEY_KEYPAD_1;
		s_KeyCodeTable[0x050]	= EKey::KEY_KEYPAD_2;
		s_KeyCodeTable[0x051]	= EKey::KEY_KEYPAD_3;
		s_KeyCodeTable[0x04B]	= EKey::KEY_KEYPAD_4;
		s_KeyCodeTable[0x04C]	= EKey::KEY_KEYPAD_5;
		s_KeyCodeTable[0x04D]	= EKey::KEY_KEYPAD_6;
		s_KeyCodeTable[0x047]	= EKey::KEY_KEYPAD_7;
		s_KeyCodeTable[0x048]	= EKey::KEY_KEYPAD_8;
		s_KeyCodeTable[0x049]	= EKey::KEY_KEYPAD_9;
		s_KeyCodeTable[0x04E]	= EKey::KEY_KEYPAD_DECIMAL;
		s_KeyCodeTable[0x053]	= EKey::KEY_KEYPAD_DIVIDE;
		s_KeyCodeTable[0x135]	= EKey::KEY_KEYPAD_MULTIPLY;
		s_KeyCodeTable[0x11C]	= EKey::KEY_KEYPAD_SUBTRACT;
		s_KeyCodeTable[0x059]	= EKey::KEY_KEYPAD_ADD;
		s_KeyCodeTable[0x037]	= EKey::KEY_KEYPAD_ENTER;
		s_KeyCodeTable[0x04A]	= EKey::KEY_KEYPAD_EQUAL;

		s_KeyCodeTable[0x02A]	= EKey::KEY_LEFT_SHIFT;
		s_KeyCodeTable[0x036]	= EKey::KEY_RIGHT_SHIFT;
		s_KeyCodeTable[0x01D]	= EKey::KEY_LEFT_CONTROL;
		s_KeyCodeTable[0x11D]	= EKey::KEY_RIGHT_CONTROL;
		s_KeyCodeTable[0x038]	= EKey::KEY_LEFT_ALT;
		s_KeyCodeTable[0x138]	= EKey::KEY_RIGHT_ALT;
		s_KeyCodeTable[0x15B]	= EKey::KEY_LEFT_SUPER;
		s_KeyCodeTable[0x15C]	= EKey::KEY_RIGHT_SUPER; 
		s_KeyCodeTable[0x15D]	= EKey::KEY_MENU;
		s_KeyCodeTable[0x039]	= EKey::KEY_SPACE;
		s_KeyCodeTable[0x028]	= EKey::KEY_APOSTROPHE;
		s_KeyCodeTable[0x033]	= EKey::KEY_COMMA;
		s_KeyCodeTable[0x00C]	= EKey::KEY_MINUS;
		s_KeyCodeTable[0x034]	= EKey::KEY_PERIOD;
		s_KeyCodeTable[0x035]	= EKey::KEY_SLASH;
		s_KeyCodeTable[0x027]	= EKey::KEY_SEMICOLON;
		s_KeyCodeTable[0x00D]	= EKey::KEY_EQUAL;
		s_KeyCodeTable[0x01A]	= EKey::KEY_LEFT_BRACKET;
		s_KeyCodeTable[0x02B]	= EKey::KEY_BACKSLASH;
		s_KeyCodeTable[0x01B]	= EKey::KEY_RIGHT_BRACKET;
		s_KeyCodeTable[0x029]	= EKey::KEY_GRAVE_ACCENT;
		s_KeyCodeTable[0x056]	= EKey::KEY_WORLD_2;
		s_KeyCodeTable[0x001]	= EKey::KEY_ESCAPE;
		s_KeyCodeTable[0x01C]	= EKey::KEY_ENTER;
		s_KeyCodeTable[0x00F]	= EKey::KEY_TAB;
		s_KeyCodeTable[0x00E]	= EKey::KEY_BACKSPACE;
		s_KeyCodeTable[0x152]	= EKey::KEY_INSERT;
		s_KeyCodeTable[0x153]	= EKey::KEY_DELETE;
		s_KeyCodeTable[0x14D]	= EKey::KEY_RIGHT;
		s_KeyCodeTable[0x14B]	= EKey::KEY_LEFT;
		s_KeyCodeTable[0x150]	= EKey::KEY_DOWN;
		s_KeyCodeTable[0x148]	= EKey::KEY_UP;
		s_KeyCodeTable[0x149]	= EKey::KEY_PAGE_UP;
		s_KeyCodeTable[0x151]	= EKey::KEY_PAGE_DOWN;
		s_KeyCodeTable[0x147]	= EKey::KEY_HOME;
		s_KeyCodeTable[0x14F]	= EKey::KEY_END;
		s_KeyCodeTable[0x03A]	= EKey::KEY_CAPS_LOCK;
		s_KeyCodeTable[0x046]	= EKey::KEY_SCROLL_LOCK;
		s_KeyCodeTable[0x145]	= EKey::KEY_NUM_LOCK;
		s_KeyCodeTable[0x137]	= EKey::KEY_PRINT_SCREEN;
		s_KeyCodeTable[0x146]	= EKey::KEY_PAUSE;

		/* Set scancode table */
		for (uint32 i = 0; i < NUM_KEY_CODES; i++)
		{
			if (s_KeyCodeTable[i] > 0)
			{
				s_ScanCodeTable[s_KeyCodeTable[i]] = i;
			}
		}

		return true;
	}

	EKey Win32InputCodeTable::GetKeyFromScanCode(uint32 scanCode)
	{
		return s_KeyCodeTable[scanCode];
	}
	
	EKey Win32InputCodeTable::GetKeyFromVirtualKey(uint32 virtualKey)
	{
		uint32 scanCode = MapVirtualKeyW(virtualKey, MAPVK_VK_TO_VSC);
		return s_KeyCodeTable[scanCode];
	}
	
	uint16 Win32InputCodeTable::GetScanCodeFromKey(EKey key)
	{
		return s_ScanCodeTable[key];
	}
	
	uint16 Win32InputCodeTable::GetVirtualKeyFromKey(EKey key)
	{
		uint16 scancode = s_ScanCodeTable[key];
		return MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX);
	}
}


#endif