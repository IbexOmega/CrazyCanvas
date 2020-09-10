#include "Input/API/Input.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

namespace LambdaEngine
{
	/*
	* Input
	*/
	bool Input::HandleEvent(const Event& event)
	{
		if (IsEventOfType<KeyPressedEvent>(event))
		{
			const KeyPressedEvent& keyEvent = EventCast<KeyPressedEvent>(event);
			s_KeyboardState.KeyStates[keyEvent.Key] = false;
		}
		else if (IsEventOfType<KeyReleasedEvent>(event))
		{
			const KeyReleasedEvent& keyEvent = EventCast<KeyReleasedEvent>(event);
			s_KeyboardState.KeyStates[keyEvent.Key] = false;
		}
		else if (IsEventOfType<MouseClickedEvent>(event))
		{
			const MouseClickedEvent& mouseEvent = EventCast<MouseClickedEvent>(event);
			s_MouseState.ButtonStates[mouseEvent.Button] = s_InputEnabled;
		}
		else if (IsEventOfType<MouseReleasedEvent>(event))
		{
			const MouseReleasedEvent& mouseEvent = EventCast<MouseReleasedEvent>(event);
			s_MouseState.ButtonStates[mouseEvent.Button] = false;
		}
		else if (IsEventOfType<MouseMovedEvent>(event))
		{
			const MouseMovedEvent& mouseEvent = EventCast<MouseMovedEvent>(event);
			s_MouseState.Position = { mouseEvent.Position.x, mouseEvent.Position.y };
		}
		else if (IsEventOfType<MouseScrolledEvent>(event))
		{
			if (s_InputEnabled)
			{
				const MouseScrolledEvent& mouseEvent = EventCast<MouseScrolledEvent>(event);
				s_MouseState.ScrollX = mouseEvent.DeltaX;
				s_MouseState.ScrollY = mouseEvent.DeltaY;
			}
		}
	}

	/*
	* Static
	*/
	bool Input::Init()
	{
		return EventQueue::RegisterEventHandler(EventHandlerProxy(Input::HandleEvent));
	}

	bool Input::Release()
	{
		return EventQueue::UnregisterEventHandler(EventHandlerProxy(Input::HandleEvent));
	}

	void Input::Tick()
	{
	}

	void Input::Disable()
	{
		s_InputEnabled = false;

		std::fill_n(s_KeyboardState.KeyStates, EKey::KEY_LAST, false);
		std::fill_n(s_MouseState.ButtonStates, EMouseButton::MOUSE_BUTTON_COUNT, false);
	}
}
