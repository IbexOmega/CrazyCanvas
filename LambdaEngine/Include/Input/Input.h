#pragma once

#include "Platform/Common/IInputDevice.h"

namespace LambdaEngine
{
	class LAMBDA_API Input
	{
	public:
		DECL_STATIC_CLASS(Input);

		static bool Init();
		static void Release();

		static void Update();

	private:
		static IInputDevice* s_pInputDevice;

		static KeyboardState s_KeyboardState;
		static MouseState s_MouseState;
	};
}
