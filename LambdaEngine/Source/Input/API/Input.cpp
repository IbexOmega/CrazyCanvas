#include "Input/API/Input.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

namespace LambdaEngine
{
	KeyboardState	Input::s_KeyboardState;
	MouseState		Input::s_MouseState;
	bool			Input::s_InputEnabled = true;

	/*
	* Input
	*/
	bool Input::HandleEvent(const Event& event)
	{
		if (IsEventOfType<KeyPressedEvent>(event))
		{
			const KeyPressedEvent& keyEvent = EventCast<KeyPressedEvent>(event);
			s_KeyboardState.KeyStates[keyEvent.Key] = true;

			return true;
		}
		else if (IsEventOfType<KeyReleasedEvent>(event))
		{
			const KeyReleasedEvent& keyEvent = EventCast<KeyReleasedEvent>(event);
			s_KeyboardState.KeyStates[keyEvent.Key] = false;

			return true;
		}
		else if (IsEventOfType<MouseClickedEvent>(event))
		{
			const MouseClickedEvent& mouseEvent = EventCast<MouseClickedEvent>(event);
			s_MouseState.ButtonStates[mouseEvent.Button] = s_InputEnabled;

			return true;
		}
		else if (IsEventOfType<MouseReleasedEvent>(event))
		{
			const MouseReleasedEvent& mouseEvent = EventCast<MouseReleasedEvent>(event);
			s_MouseState.ButtonStates[mouseEvent.Button] = false;

			return true;
		}
		else if (IsEventOfType<MouseMovedEvent>(event))
		{
			const MouseMovedEvent& mouseEvent = EventCast<MouseMovedEvent>(event);
			s_MouseState.Position = { mouseEvent.Position.x, mouseEvent.Position.y };

			return true;
		}
		else if (IsEventOfType<MouseScrolledEvent>(event))
		{
			if (s_InputEnabled)
			{
				const MouseScrolledEvent& mouseEvent = EventCast<MouseScrolledEvent>(event);
				s_MouseState.ScrollX = mouseEvent.DeltaX;
				s_MouseState.ScrollY = mouseEvent.DeltaY;
			}

			return true;
		}

		return false;
	}

	/*
	* Static
	*/
	bool Input::Init()
	{
		return EventQueue::RegisterEventHandler(EventHandler(FEventFlag::EVENT_FLAG_INPUT, Input::HandleEvent));
	}

	bool Input::Release()
	{
		return EventQueue::UnregisterEventHandler(EventHandler(FEventFlag::EVENT_FLAG_INPUT, Input::HandleEvent));
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
