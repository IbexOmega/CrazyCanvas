#pragma once
#include "InputState.h"

#include "Application/API/EventHandler.h"

namespace LambdaEngine
{
	class LAMBDA_API Input : public EventHandler
	{
    private:
        Input()     = default;
        ~Input()    = default;

    public:
        virtual void OnButtonPressed(EMouseButton button, uint32 modifierMask)    override;
        virtual void OnButtonReleased(EMouseButton button)                        override;
        virtual void OnMouseMoved(int32 x, int32 y)                               override;
        virtual void OnMouseScrolled(int32 deltaX, int32 deltaY)                  override;

        virtual void OnKeyPressed(EKey key, uint32 modifierMask, bool isRepeat)   override;
        virtual void OnKeyReleased(EKey key)                                      override;

	public:
        DECL_UNIQUE_CLASS(Input);

		static bool Init();
		static void Release();

		static void Tick();

        static void Enable() { s_pInstance->m_InputEnabled = true; };
        static void Disable();

        FORCEINLINE static bool IsKeyDown(EKey key)
        {
            return s_pInstance->m_KeyboardState.IsKeyDown(key);
        }

        FORCEINLINE static bool IsKeyUp(EKey key)
        {
            return s_pInstance->m_KeyboardState.IsKeyUp(key);
        }

		FORCEINLINE static const KeyboardState& GetKeyboardState()
        {
            return s_pInstance->m_KeyboardState;
        }

		FORCEINLINE static const MouseState& GetMouseState()
        {
            return s_pInstance->m_MouseState;
        }

    private:
        KeyboardState	m_KeyboardState = { };
		MouseState		m_MouseState    = { };
        bool m_InputEnabled = true;

    private:
		static Input* s_pInstance;
	};
}
