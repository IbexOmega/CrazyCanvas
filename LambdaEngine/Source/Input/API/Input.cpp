#include "Input/API/Input.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

namespace LambdaEngine
{
	KeyboardState	Input::s_KeyboardStates[2];
	MouseState		Input::s_MouseStates[2];
	bool			Input::s_InputEnabled = true;
	SpinLock		Input::s_WriteBufferLockMouse;
	SpinLock		Input::s_WriteBufferLockKeyboard;

	/*
	* Input
	*/
	bool Input::HandleEvent(const Event& event)
	{
		if (s_InputEnabled)
		{
			if (IsEventOfType<KeyPressedEvent>(event))
			{
				std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
				const KeyPressedEvent& keyEvent = EventCast<KeyPressedEvent>(event);
				s_KeyboardStates[STATE_WRITE_INDEX].KeyStates[keyEvent.Key] = true;

				return true;
			}
			else if (IsEventOfType<KeyReleasedEvent>(event))
			{
				std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
				const KeyReleasedEvent& keyEvent = EventCast<KeyReleasedEvent>(event);
				s_KeyboardStates[STATE_WRITE_INDEX].KeyStates[keyEvent.Key] = false;

				return true;
			}
			else if (IsEventOfType<MouseButtonClickedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseButtonClickedEvent& mouseEvent = EventCast<MouseButtonClickedEvent>(event);
				s_MouseStates[STATE_WRITE_INDEX].ButtonStates[mouseEvent.Button] = s_InputEnabled;

				return true;
			}
			else if (IsEventOfType<MouseButtonReleasedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseButtonReleasedEvent& mouseEvent = EventCast<MouseButtonReleasedEvent>(event);
				s_MouseStates[STATE_WRITE_INDEX].ButtonStates[mouseEvent.Button] = false;

				return true;
			}
			else if (IsEventOfType<MouseMovedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseMovedEvent& mouseEvent = EventCast<MouseMovedEvent>(event);
				s_MouseStates[STATE_WRITE_INDEX].Position = { mouseEvent.Position.x, mouseEvent.Position.y };

				return true;
			}
			else if (IsEventOfType<MouseScrolledEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseScrolledEvent& mouseEvent = EventCast<MouseScrolledEvent>(event);
				s_MouseStates[STATE_WRITE_INDEX].ScrollX = mouseEvent.DeltaX;
				s_MouseStates[STATE_WRITE_INDEX].ScrollY = mouseEvent.DeltaY;

				return true;
			}
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
		result = result && EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseButtonReleasedEvent>(eventHandler);
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
		result = result && EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseButtonReleasedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseMovedEvent>(eventHandler);
		result = result && EventQueue::UnregisterEventHandler<MouseScrolledEvent>(eventHandler);
		return result;
	}

	void Input::Tick()
	{
		std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
		std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);

		s_KeyboardStates[0] = s_KeyboardStates[1];
		s_MouseStates[0] = s_MouseStates[1];
	}

	void Input::Disable()
	{
		std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
		std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
		s_InputEnabled = false;

		std::fill_n(s_KeyboardStates[STATE_WRITE_INDEX].KeyStates, EKey::KEY_COUNT, false);
		std::fill_n(s_MouseStates[STATE_WRITE_INDEX].ButtonStates, EMouseButton::MOUSE_BUTTON_COUNT, false);
	}
}
