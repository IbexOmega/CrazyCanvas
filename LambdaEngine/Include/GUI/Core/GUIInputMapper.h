#pragma once

#include "Input/API/Input.h"

#include "NsGui/InputEnums.h"

namespace LambdaEngine
{
	class GUIInputMapper
	{
	public:
		DECL_STATIC_CLASS(GUIInputMapper);

		static bool Init();

		static Noesis::Key GetKey(EKey key);
		static Noesis::MouseButton GetMouseButton(EMouseButton button);

	private:
		static Noesis::Key s_KeyMapping[EKey::KEY_COUNT];
		static Noesis::MouseButton s_MouseButtonMapping[EMouseButton::MOUSE_BUTTON_COUNT + 1];
	};
}