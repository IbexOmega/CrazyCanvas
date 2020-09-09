#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	/*
	* EKey
	*/
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
	
	/*
	* FModifierFlag
	*/
	enum FModifierFlag
	{
		MODIFIER_FLAG_NONE       = 0,
		MODIFIER_FLAG_CTRL       = FLAG(1),
		MODIFIER_FLAG_ALT        = FLAG(2),
		MODIFIER_FLAG_SHIFT      = FLAG(3),
		MODIFIER_FLAG_CAPS_LOCK  = FLAG(4),
		MODIFIER_FLAG_SUPER      = FLAG(5),
		MODIFIER_FLAG_NUM_LOCK   = FLAG(6),
	};

	/*
	* EMouseButton
	*/
	enum EMouseButton : uint8
	{
		MOUSE_BUTTON_UNKNOWN    = 0,

		MOUSE_BUTTON_LEFT       = 1,
		MOUSE_BUTTON_RIGHT      = 2,
		MOUSE_BUTTON_MIDDLE     = 3,
		MOUSE_BUTTON_BACK       = 4,
		MOUSE_BUTTON_FORWARD    = 5,

		MOUSE_BUTTON_LAST   = MOUSE_BUTTON_BACK,
		MOUSE_BUTTON_COUNT  = MOUSE_BUTTON_LAST + 1
	};

	/*
	* Helpers
	*/
	inline const char* KeyToString(EKey key)
	{
		switch (key)
		{
			case KEY_0:					return "0";
			case KEY_1:					return "1";
			case KEY_2:					return "2";
			case KEY_3:					return "3";
			case KEY_4:					return "4";
			case KEY_5:					return "5";
			case KEY_6:					return "6";
			case KEY_7:					return "7";
			case KEY_8:					return "8";
			case KEY_9:					return "9";
			case KEY_A:					return "A";
			case KEY_B:					return "B";
			case KEY_C:					return "C";
			case KEY_D:					return "D";
			case KEY_E:					return "E";
			case KEY_F:					return "F";
			case KEY_G:					return "G";
			case KEY_H:					return "H";
			case KEY_I:					return "I";
			case KEY_J:					return "J";
			case KEY_K:					return "K";
			case KEY_L:					return "L";
			case KEY_M:					return "M";
			case KEY_N:					return "N";
			case KEY_O:					return "O";
			case KEY_P:					return "P";
			case KEY_Q:					return "Q";
			case KEY_R:					return "R";
			case KEY_S:					return "S";
			case KEY_T:					return "T";
			case KEY_U:					return "U";
			case KEY_V:					return "V";
			case KEY_W:					return "W";
			case KEY_X:					return "X";
			case KEY_Y:					return "Y";
			case KEY_Z:					return "Z";
			case KEY_F1:				return "F1";
			case KEY_F2:				return "F2";
			case KEY_F3:				return "F3";
			case KEY_F4:				return "F4";
			case KEY_F5:				return "F5";
			case KEY_F6:				return "F6";
			case KEY_F7:				return "F7";
			case KEY_F8:				return "F8";
			case KEY_F9:				return "F9";
			case KEY_F10:				return "F10";
			case KEY_F11:				return "F11";
			case KEY_F12:				return "F12";
			case KEY_F13:				return "F13";
			case KEY_F14:				return "F14";
			case KEY_F15:				return "F15";
			case KEY_F16:				return "F16";
			case KEY_F17:				return "F17";
			case KEY_F18:				return "F18";
			case KEY_F19:				return "F19";
			case KEY_F20:				return "F20";
			case KEY_F21:				return "F21";
			case KEY_F22:				return "F22";
			case KEY_F23:				return "F23";
			case KEY_F24:				return "F24";
			case KEY_F25:				return "F25";
			case KEY_KEYPAD_0:			return "KEYPAD_0";
			case KEY_KEYPAD_1:			return "KEYPAD_1";
			case KEY_KEYPAD_2:			return "KEYPAD_2";
			case KEY_KEYPAD_3:			return "KEYPAD_3";
			case KEY_KEYPAD_4:			return "KEYPAD_4";
			case KEY_KEYPAD_5:			return "KEYPAD_5";
			case KEY_KEYPAD_6:			return "KEYPAD_6";
			case KEY_KEYPAD_7:			return "KEYPAD_7";
			case KEY_KEYPAD_8:			return "KEYPAD_8";
			case KEY_KEYPAD_9:			return "KEYPAD_9";
			case KEY_KEYPAD_DECIMAL:	return "KEYPAD_DECIMAL";
			case KEY_KEYPAD_DIVIDE:		return "KEYPAD_DIVIDE";
			case KEY_KEYPAD_MULTIPLY:	return "KEYPAD_MULTIPLY";
			case KEY_KEYPAD_SUBTRACT:	return "KEYPAD_SUBTRACT";
			case KEY_KEYPAD_ADD:		return "KEYPAD_ADD";
			case KEY_KEYPAD_ENTER:		return "KEYPAD_ENTER";
			case KEY_KEYPAD_EQUAL:		return "KEYPAD_EQUAL";
			case KEY_LEFT_SHIFT:		return "LEFT_SHIFT";
			case KEY_LEFT_CONTROL:		return "LEFT_CONTROL";
			case KEY_LEFT_ALT:			return "LEFT_ALT";
			case KEY_LEFT_SUPER:		return "LEFT_SUPER";
			case KEY_RIGHT_SHIFT:		return "RIGHT_SHIFT";
			case KEY_RIGHT_CONTROL:		return "RIGHT_CONTROL";
			case KEY_RIGHT_ALT:			return "RIGHT_ALT";
			case KEY_RIGHT_SUPER:		return "RIGHT_SUPER";
			case KEY_MENU:				return "MENU";
			case KEY_SPACE:				return "SPACE";
			case KEY_APOSTROPHE:		return "APOSTROPHE";
			case KEY_COMMA:				return "COMMA";
			case KEY_MINUS:				return "MINUS";
			case KEY_PERIOD:			return "PERIOD";
			case KEY_SLASH:				return "SLASH";
			case KEY_SEMICOLON:			return "SEMICOLON";
			case KEY_EQUAL:				return "EQUAL";
			case KEY_LEFT_BRACKET:		return "LEFT_BRACKET";
			case KEY_BACKSLASH:			return "BACKSLASH";
			case KEY_RIGHT_BRACKET:		return "RIGHT_BRACKET";
			case KEY_GRAVE_ACCENT:		return "GRAVE_ACCENT";
			case KEY_WORLD_1:			return "WORLD_1";
			case KEY_WORLD_2:			return "WORLD_2";
			case KEY_ESCAPE:			return "ESCAPE";
			case KEY_ENTER:				return "ENTER";
			case KEY_TAB:				return "TAB";
			case KEY_BACKSPACE:			return "BACKSPACE";
			case KEY_INSERT:			return "INSERT";
			case KEY_DELETE:			return "DELETE";
			case KEY_RIGHT:				return "RIGHT";
			case KEY_LEFT:				return "LEFT";
			case KEY_DOWN:				return "DOWN";
			case KEY_UP:				return "UP";
			case KEY_PAGE_UP:			return "PAGE_UP";
			case KEY_PAGE_DOWN:			return "PAGE_DOWN";
			case KEY_HOME:				return "HOME";
			case KEY_END:				return "END";
			case KEY_CAPS_LOCK:			return "CAPS_LOCK";
			case KEY_SCROLL_LOCK:		return "SCROLL_LOCK";
			case KEY_NUM_LOCK:			return "NUM_LOCK";
			case KEY_PRINT_SCREEN:		return "PRINT_SCREEN";
			case KEY_PAUSE:				return "PAUSE";
			default:					return "UNKNOWN";
		}
	}
}
