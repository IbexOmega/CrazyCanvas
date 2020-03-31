#pragma once
#include "InputCodes.h"

namespace LambdaEngine
{
	class IKeyboardHandler
	{
	public:
		DECL_INTERFACE(IKeyboardHandler);
	
		virtual void OnKeyDown(EKey key)		= 0;
		virtual void OnKeyHeldDown(EKey key)	= 0;
		virtual void OnKeyUp(EKey key)			= 0;
	};
}
