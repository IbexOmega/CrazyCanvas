#ifdef LAMBDA_PLATFORM_MACOS
#include "Platform/macOS/MacInputCodeTable.h"

#include <AppKit/AppKit.h>

namespace LambdaEngine
{
    EKey             MacInputCodeTable::s_KeyCodeTable[256];
    EMouseButton     MacInputCodeTable::s_MouseButtonCodeTable[5];

    bool MacInputCodeTable::Init()
    {
        ZERO_MEMORY(s_KeyCodeTable, sizeof(s_KeyCodeTable));
        s_KeyCodeTable[0x33] = EKey::KEY_BACKSPACE;
        s_KeyCodeTable[0x30] = EKey::KEY_TAB;
        s_KeyCodeTable[0x24] = EKey::KEY_ENTER;
        s_KeyCodeTable[0x39] = EKey::KEY_CAPS_LOCK;
        s_KeyCodeTable[0x31] = EKey::KEY_SPACE;
        s_KeyCodeTable[0x74] = EKey::KEY_PAGE_UP;
        s_KeyCodeTable[0x79] = EKey::KEY_PAGE_DOWN;
        s_KeyCodeTable[0x77] = EKey::KEY_END;
        s_KeyCodeTable[0x73] = EKey::KEY_HOME;
        s_KeyCodeTable[0x7B] = EKey::KEY_LEFT;
        s_KeyCodeTable[0x7E] = EKey::KEY_UP;
        s_KeyCodeTable[0x7C] = EKey::KEY_RIGHT;
        s_KeyCodeTable[0x7D] = EKey::KEY_DOWN;
        s_KeyCodeTable[0x72] = EKey::KEY_INSERT;
        s_KeyCodeTable[0x75] = EKey::KEY_DELETE;
        s_KeyCodeTable[0x35] = EKey::KEY_ESCAPE;
        s_KeyCodeTable[0x1D] = EKey::KEY_0;
        s_KeyCodeTable[0x12] = EKey::KEY_1;
        s_KeyCodeTable[0x13] = EKey::KEY_2;
        s_KeyCodeTable[0x14] = EKey::KEY_3;
        s_KeyCodeTable[0x15] = EKey::KEY_4;
        s_KeyCodeTable[0x17] = EKey::KEY_5;
        s_KeyCodeTable[0x16] = EKey::KEY_6;
        s_KeyCodeTable[0x1A] = EKey::KEY_7;
        s_KeyCodeTable[0x1C] = EKey::KEY_8;
        s_KeyCodeTable[0x19] = EKey::KEY_9;
        s_KeyCodeTable[0x00] = EKey::KEY_A;
        s_KeyCodeTable[0x0B] = EKey::KEY_B;
        s_KeyCodeTable[0x08] = EKey::KEY_C;
        s_KeyCodeTable[0x02] = EKey::KEY_D;
        s_KeyCodeTable[0x0E] = EKey::KEY_E;
        s_KeyCodeTable[0x03] = EKey::KEY_F;
        s_KeyCodeTable[0x05] = EKey::KEY_G;
        s_KeyCodeTable[0x04] = EKey::KEY_H;
        s_KeyCodeTable[0x22] = EKey::KEY_I;
        s_KeyCodeTable[0x26] = EKey::KEY_J;
        s_KeyCodeTable[0x28] = EKey::KEY_K;
        s_KeyCodeTable[0x25] = EKey::KEY_L;
        s_KeyCodeTable[0x2E] = EKey::KEY_M;
        s_KeyCodeTable[0x2D] = EKey::KEY_N;
        s_KeyCodeTable[0x1F] = EKey::KEY_O;
        s_KeyCodeTable[0x23] = EKey::KEY_P;
        s_KeyCodeTable[0x0C] = EKey::KEY_Q;
        s_KeyCodeTable[0x0F] = EKey::KEY_R;
        s_KeyCodeTable[0x01] = EKey::KEY_S;
        s_KeyCodeTable[0x11] = EKey::KEY_T;
        s_KeyCodeTable[0x20] = EKey::KEY_U;
        s_KeyCodeTable[0x09] = EKey::KEY_V;
        s_KeyCodeTable[0x0D] = EKey::KEY_W;
        s_KeyCodeTable[0x07] = EKey::KEY_X;
        s_KeyCodeTable[0x10] = EKey::KEY_Y;
        s_KeyCodeTable[0x06] = EKey::KEY_Z;
        s_KeyCodeTable[0x52] = EKey::KEY_KP_0;
        s_KeyCodeTable[0x53] = EKey::KEY_KP_1;
        s_KeyCodeTable[0x54] = EKey::KEY_KP_2;
        s_KeyCodeTable[0x55] = EKey::KEY_KP_3;
        s_KeyCodeTable[0x56] = EKey::KEY_KP_4;
        s_KeyCodeTable[0x57] = EKey::KEY_KP_5;
        s_KeyCodeTable[0x58] = EKey::KEY_KP_6;
        s_KeyCodeTable[0x59] = EKey::KEY_KP_7;
        s_KeyCodeTable[0x5B] = EKey::KEY_KP_8;
        s_KeyCodeTable[0x5C] = EKey::KEY_KP_9;
        s_KeyCodeTable[0x45] = EKey::KEY_KP_ADD;
        s_KeyCodeTable[0x41] = EKey::KEY_KP_DECIMAL;
        s_KeyCodeTable[0x4B] = EKey::KEY_KP_DIVIDE;
        s_KeyCodeTable[0x4C] = EKey::KEY_KP_ENTER;
        s_KeyCodeTable[0x51] = EKey::KEY_KP_EQUAL;
        s_KeyCodeTable[0x43] = EKey::KEY_KP_MULTIPLY;
        s_KeyCodeTable[0x4E] = EKey::KEY_KP_SUBTRACT;
        s_KeyCodeTable[0x7A] = EKey::KEY_F1;
        s_KeyCodeTable[0x78] = EKey::KEY_F2;
        s_KeyCodeTable[0x63] = EKey::KEY_F3;
        s_KeyCodeTable[0x76] = EKey::KEY_F4;
        s_KeyCodeTable[0x60] = EKey::KEY_F5;
        s_KeyCodeTable[0x61] = EKey::KEY_F6;
        s_KeyCodeTable[0x62] = EKey::KEY_F7;
        s_KeyCodeTable[0x64] = EKey::KEY_F8;
        s_KeyCodeTable[0x65] = EKey::KEY_F9;
        s_KeyCodeTable[0x6D] = EKey::KEY_F10;
        s_KeyCodeTable[0x67] = EKey::KEY_F11;
        s_KeyCodeTable[0x6F] = EKey::KEY_F12;
        s_KeyCodeTable[0x69] = EKey::KEY_F13;
        s_KeyCodeTable[0x6B] = EKey::KEY_F14;
        s_KeyCodeTable[0x71] = EKey::KEY_F15;
        s_KeyCodeTable[0x6A] = EKey::KEY_F16;
        s_KeyCodeTable[0x40] = EKey::KEY_F17;
        s_KeyCodeTable[0x4F] = EKey::KEY_F18;
        s_KeyCodeTable[0x50] = EKey::KEY_F19;
        s_KeyCodeTable[0x5A] = EKey::KEY_F20;
        s_KeyCodeTable[0x47] = EKey::KEY_NUM_LOCK;
        s_KeyCodeTable[0x29] = EKey::KEY_SEMICOLON;
        s_KeyCodeTable[0x2B] = EKey::KEY_COMMA;
        s_KeyCodeTable[0x1B] = EKey::KEY_MINUS;
        s_KeyCodeTable[0x2F] = EKey::KEY_PERIOD;
        s_KeyCodeTable[0x32] = EKey::KEY_GRAVE_ACCENT;
        s_KeyCodeTable[0x21] = EKey::KEY_LEFT_BRACKET;
        s_KeyCodeTable[0x1E] = EKey::KEY_RIGHT_BRACKET;
        s_KeyCodeTable[0x27] = EKey::KEY_APOSTROPHE;
        s_KeyCodeTable[0x2A] = EKey::KEY_BACKSLASH;
        s_KeyCodeTable[0x38] = EKey::KEY_LEFT_SHIFT;
        s_KeyCodeTable[0x3B] = EKey::KEY_LEFT_CONTROL;
        s_KeyCodeTable[0x3A] = EKey::KEY_LEFT_ALT;
        s_KeyCodeTable[0x37] = EKey::KEY_LEFT_SUPER;
        s_KeyCodeTable[0x3C] = EKey::KEY_RIGHT_SHIFT;
        s_KeyCodeTable[0x3E] = EKey::KEY_RIGHT_CONTROL;
        s_KeyCodeTable[0x36] = EKey::KEY_RIGHT_SUPER;
        s_KeyCodeTable[0x3D] = EKey::KEY_RIGHT_ALT;
        s_KeyCodeTable[0x6E] = EKey::KEY_MENU;
        s_KeyCodeTable[0x18] = EKey::KEY_EQUAL;
        s_KeyCodeTable[0x2C] = EKey::KEY_SLASH;
        s_KeyCodeTable[0x0A] = EKey::KEY_WORLD_1;
        
        ZERO_MEMORY(s_MouseButtonCodeTable, sizeof(s_MouseButtonCodeTable));
        s_MouseButtonCodeTable[0] = EMouseButton::MOUSE_BUTTON_LEFT;
        s_MouseButtonCodeTable[1] = EMouseButton::MOUSE_BUTTON_RIGHT;
        s_MouseButtonCodeTable[2] = EMouseButton::MOUSE_BUTTON_MIDDLE;
        
        return true;
    }

    EKey MacInputCodeTable::GetKey(int32 keyCode)
    {
        return s_KeyCodeTable[keyCode];
    }

    EMouseButton MacInputCodeTable::GetMouseButton(int32 mouseButton)
    {
        return s_MouseButtonCodeTable[mouseButton];
    }
}

#endif
