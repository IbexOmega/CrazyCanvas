#pragma once
#include "IInputDevice.h"

#include "Containers/TArray.h"

namespace LambdaEngine
{
	class InputDeviceBase : public IInputDevice
	{
	public:
		DECL_ABSTRACT_CLASS(InputDeviceBase);

        virtual KeyboardState   GetKeyboardState()  const override;
        virtual MouseState      GetMouseState()     const override;

		virtual void AddKeyboardHandler(IKeyboardHandler* pHandler) override;
		virtual void AddMouseHandler(IMouseHandler* pHandler) 		override;

		virtual void RemoveKeyboardHandler(IKeyboardHandler* pHandler) 	override;
		virtual void RemoveMouseHandler(IMouseHandler* pHandler) 		override;

    protected:
		void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat);
		void OnKeyReleased(EKey key);
        void OnKeyTyped(uint32 character);
        
		void OnMouseMoved(int32 x, int32 y);
		void OnMouseButtonPressed(EMouseButton button, uint32 modifierMask);
		void OnMouseButtonReleased(EMouseButton button);
		void OnMouseScrolled(int32 deltaX, int32 deltaY);

	private:
		KeyboardState	m_KeyboardState;
		MouseState		m_MouseState;

		TArray<IKeyboardHandler*>	m_KeyboardHandlers;
		TArray<IMouseHandler*>		m_MouseHandlers;

	};
}
