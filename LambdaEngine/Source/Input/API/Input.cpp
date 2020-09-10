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
		EventHandler eventHandler(Input::HandleEvent);

		bool result = true;
		result = result && EventQueue::RegisterEventHandler<KeyPressedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<KeyReleasedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseClickedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseReleasedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseMovedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseScrolledEvent>(eventHandler);
		return result;
	}

	bool Input::Release()
	{
		EventHandler eventHandler(Input::HandleEvent);

		bool result = true;
		result = result && EventQueue::UnregisterEventHandler<KeyPressedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<KeyReleasedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseClickedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseReleasedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseMovedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseScrolledEvent>(eventHandler);
		return result;
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
