#pragma once
#include "Application/API/InputDevice.h"

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

		static void Update();

		static void AddKeyboardHandler(IKeyboardHandler* pHandler);
		static void AddMouseHandler(IMouseHandler* pHandler);

	private:
		static InputDevice* s_pInputDevice;

		static KeyboardState s_KeyboardState;
		static MouseState s_MouseState;
	};
}
