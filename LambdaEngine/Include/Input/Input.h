#pragma once

#include "Platform/Common/InputDevice.h"

namespace LambdaEngine
{
	class IKeyboardHandler;
	class IMouseHandler;

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
