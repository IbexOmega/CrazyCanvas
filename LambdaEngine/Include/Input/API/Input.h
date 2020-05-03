#pragma once
#include "IInputDevice.h"

#include "Application/API/PlatformApplication.h"

namespace LambdaEngine
{
    class IMouseHandler;
    class IKeyboardHandler;

	class LAMBDA_API Input
	{
	public:
		DECL_STATIC_CLASS(Input);

		static bool Init();
		static void Release();

		static void Tick();

		static void AddKeyboardHandler(IKeyboardHandler* pHandler);
		static void AddMouseHandler(IMouseHandler* pHandler);

		static void SetInputMode(EInputMode inputMode);
        
        FORCEINLINE static bool IsKeyDown(EKey key)
        {
            return s_KeyboardState.IsKeyDown(key);
        }
        
        FORCEINLINE static bool IsKeyUp(EKey key)
        {
            return s_KeyboardState.IsKeyUp(key);
        }

		FORCEINLINE static const KeyboardState& GetKeyboardState()
        {
            return s_KeyboardState;
        }
        
		FORCEINLINE static const MouseState& GetMouseState()
        {
            return s_MouseState;
        }

	private:
		static IInputDevice* s_pInputDevice;

		static KeyboardState	s_KeyboardState;
		static MouseState		s_MouseState;
	};
}
