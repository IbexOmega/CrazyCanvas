#pragma once

#include "InputCodes.h"

namespace LambdaEngine
{
	class IMouseHandler
	{
	public:
		DECL_INTERFACE(IMouseHandler);

		virtual void OnMouseMove(int32 x, int32 y) = 0;

		virtual void OnButtonPressed(EMouseButton button) = 0;
		virtual void OnButtonReleased(EMouseButton button) = 0;

		virtual void OnScroll(int32 delta) = 0;
	};
}