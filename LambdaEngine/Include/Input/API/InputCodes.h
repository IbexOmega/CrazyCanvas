#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	enum EKey : uint8
	{	
		KEY_UNKNOWN = 0,

		/* Numbers */
		KEY_0 = 7,
		KEY_1 = 8,
		KEY_2 = 9,
		KEY_3 = 10,
		KEY_4 = 11,
		KEY_5 = 12,
		KEY_6 = 13,
		KEY_7 = 14,
		KEY_8 = 15,
		KEY_9 = 16,
		
		/* Letters */
		KEY_A = 19,
		KEY_B = 20,
		KEY_C = 21,
		KEY_D = 22,
		KEY_E = 23,
		KEY_F = 24,
		KEY_G = 25,
		KEY_H = 26,
		KEY_I = 27,
		KEY_J = 28,
		KEY_K = 29,
		KEY_L = 30,
		KEY_M = 31,
		KEY_N = 32,
		KEY_O = 33,
		KEY_P = 34,
		KEY_Q = 35,
		KEY_R = 36,
		KEY_S = 37,
		KEY_T = 38,
		KEY_U = 39,
		KEY_V = 40,
		KEY_W = 41,
		KEY_X = 42,
		KEY_Y = 43,
		KEY_Z = 44,
		
		/* Function keys */
		KEY_F1	= 70,
		KEY_F2	= 71,
		KEY_F3	= 72,
		KEY_F4	= 73,
		KEY_F5	= 74,
		KEY_F6	= 75,
		KEY_F7	= 76,
		KEY_F8	= 77,
		KEY_F9	= 78,
		KEY_F10	= 79,
		KEY_F11	= 80,
		KEY_F12	= 81,
		KEY_F13	= 82,
		KEY_F14	= 83,
		KEY_F15	= 84,
		KEY_F16	= 85,
		KEY_F17	= 86,
		KEY_F18	= 87,
		KEY_F19	= 88,
		KEY_F20	= 89,
		KEY_F21	= 90,
		KEY_F22	= 91,
		KEY_F23	= 92,
		KEY_F24	= 93,
		KEY_F25	= 94,

		/* Keypad */
		KEY_KEYPAD_0        = 95,
		KEY_KEYPAD_1        = 96,
		KEY_KEYPAD_2        = 97,
		KEY_KEYPAD_3        = 98,
		KEY_KEYPAD_4        = 99,
		KEY_KEYPAD_5        = 100,
		KEY_KEYPAD_6        = 101,
		KEY_KEYPAD_7        = 102,
		KEY_KEYPAD_8        = 103,
		KEY_KEYPAD_9        = 104,
		KEY_KEYPAD_DECIMAL  = 105,
		KEY_KEYPAD_DIVIDE   = 106,
		KEY_KEYPAD_MULTIPLY = 107,
		KEY_KEYPAD_SUBTRACT = 108,
		KEY_KEYPAD_ADD      = 109,
		KEY_KEYPAD_ENTER    = 110,
		KEY_KEYPAD_EQUAL    = 111,

		/* Ctrl, Shift, Alt, etc.*/
		KEY_LEFT_SHIFT	  = 112,
		KEY_LEFT_CONTROL  = 113,
		KEY_LEFT_ALT	  = 114,
		KEY_LEFT_SUPER	  = 115,
		KEY_RIGHT_SHIFT	  = 116,
		KEY_RIGHT_CONTROL = 117,
		KEY_RIGHT_ALT	  = 118,
		KEY_RIGHT_SUPER	  = 119,
		KEY_MENU		  = 120,

		/* Other */
		KEY_SPACE		  = 1,
		KEY_APOSTROPHE	  = 2,  	/* ' */
		KEY_COMMA		  = 3,  	/* , */
		KEY_MINUS		  = 4,  	/* - */
		KEY_PERIOD		  = 5,  	/* . */
		KEY_SLASH		  = 6,  	/* / */
		KEY_SEMICOLON	  = 17,  	/* ; */
		KEY_EQUAL		  = 18,  	/* = */
		KEY_LEFT_BRACKET  = 45,		/* [ */
		KEY_BACKSLASH	  = 46,		/* \ */
		KEY_RIGHT_BRACKET = 47,		/* ] */
		KEY_GRAVE_ACCENT  = 48,		/* ` */
		KEY_WORLD_1		  = 49,		/* non-US #1 */
		KEY_WORLD_2		  = 50,		/* non-US #2 */
		KEY_ESCAPE		  = 51,
		KEY_ENTER		  = 52,
		KEY_TAB			  = 53,
		KEY_BACKSPACE	  = 54,
		KEY_INSERT		  = 55,
		KEY_DELETE		  = 56,
		KEY_RIGHT		  = 57,
		KEY_LEFT		  = 58,
		KEY_DOWN		  = 59,
		KEY_UP			  = 60,
		KEY_PAGE_UP		  = 61,
		KEY_PAGE_DOWN	  = 62,
		KEY_HOME		  = 63,
		KEY_END			  = 64,
		KEY_CAPS_LOCK	  = 65,
		KEY_SCROLL_LOCK	  = 66,
		KEY_NUM_LOCK	  = 67,
		KEY_PRINT_SCREEN  = 68,
		KEY_PAUSE		  = 69,

		KEY_LAST    = KEY_MENU,
		KEY_COUNT   = KEY_LAST + 1
	};

	enum EMouseButton : uint8
	{
		MOUSE_BUTTON_UNKNOWN    = 0,

		MOUSE_BUTTON_LEFT       = 1,
		MOUSE_BUTTON_MIDDLE     = 2,
		MOUSE_BUTTON_RIGHT      = 3,
		MOUSE_BUTTON_BACK       = 4,
		MOUSE_BUTTON_FORWARD    = 5,

		MOUSE_BUTTON_LAST   = MOUSE_BUTTON_BACK,
		MOUSE_BUTTON_COUNT  = MOUSE_BUTTON_LAST + 1
	};
}
