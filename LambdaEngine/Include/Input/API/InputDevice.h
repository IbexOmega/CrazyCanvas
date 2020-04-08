#pragma once
#include "IInputDevice.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class InputDevice : public IInputDevice
	{
	public:
		DECL_ABSTRACT_CLASS(InputDevice);

		virtual KeyboardState GetKeyboardState() const override 
		{ 
			return m_KeyboardState; 
		}

		virtual MouseState GetMouseState() const override
		{ 
			return m_MouseState; 
		}

		virtual void AddKeyboardHandler(IKeyboardHandler* pHandler) override;
		virtual void AddMouseHandler(IMouseHandler* pHandler) 		override;

		virtual void RemoveKeyboardHandler(IKeyboardHandler* pHandler) 	override;
		virtual void RemoveMouseHandler(IMouseHandler* pHandler) 		override;

		void OnKeyDown(EKey key);
		void OnKeyHeldDown(EKey key);
		void OnKeyUp(EKey key);

		void OnMouseMove(int32 x, int32 y);
		void OnMouseButtonPressed(EMouseButton button);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseScrolled(int32 delta);

	private:
		KeyboardState	m_KeyboardState;
		MouseState		m_MouseState;

		std::vector<IKeyboardHandler*>	m_KeyboardHandlers;
		std::vector<IMouseHandler*>		m_MouseHandlers;

	};
}
