#include "Input/API/Input.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"
#include "Application/API/Events/WindowEvents.h"

namespace LambdaEngine
{
	KeyboardState			Input::s_KeyboardStates[2][2];
	MouseState				Input::s_MouseStates[2][2];
	std::atomic_bool		Input::s_InputEnabled = true;
	SpinLock				Input::s_WriteBufferLockMouse;
	SpinLock				Input::s_WriteBufferLockKeyboard;
	std::stack<InputMode>	Input::s_InputModeStack;

	/*
	* Input
	*/
	bool Input::HandleEvent(const Event& event)
	{
		// Disable or enable based on if the window is active
		if (IsEventOfType<WindowFocusChangedEvent>(event))
		{
			const WindowFocusChangedEvent& focusEvent = EventCast<WindowFocusChangedEvent>(event);
			if (focusEvent.HasFocus)
			{
				Enable();
			}
			else
			{
				Disable();
			}

			return true;
		}

		// Update input
		if (IsInputEnabled() && GetCurrentInputmode() != InputMode::NONE)
		{
			uint8 inputMode = ConvertInputModeUINT8(s_InputModeStack.top());

			if (IsEventOfType<KeyPressedEvent>(event))
			{
				std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
				const KeyPressedEvent& keyEvent = EventCast<KeyPressedEvent>(event);
				s_KeyboardStates[inputMode][STATE_WRITE_INDEX].KeyStates[keyEvent.Key] = true;

				return true;
			}
			else if (IsEventOfType<KeyReleasedEvent>(event))
			{
				std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
				const KeyReleasedEvent& keyEvent = EventCast<KeyReleasedEvent>(event);
				s_KeyboardStates[inputMode][STATE_WRITE_INDEX].KeyStates[keyEvent.Key] = false;

				return true;
			}
			else if (IsEventOfType<MouseButtonClickedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseButtonClickedEvent& mouseEvent = EventCast<MouseButtonClickedEvent>(event);
				s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates[mouseEvent.Button] = IsInputEnabled();

				return true;
			}
			else if (IsEventOfType<MouseButtonReleasedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseButtonReleasedEvent& mouseEvent = EventCast<MouseButtonReleasedEvent>(event);
				s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates[mouseEvent.Button] = false;

				return true;
			}
			else if (IsEventOfType<MouseMovedEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseMovedEvent& mouseEvent = EventCast<MouseMovedEvent>(event);
				s_MouseStates[inputMode][STATE_WRITE_INDEX].Position = { mouseEvent.Position.x, mouseEvent.Position.y };

				return true;
			}
			else if (IsEventOfType<MouseScrolledEvent>(event))
			{
				std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
				const MouseScrolledEvent& mouseEvent = EventCast<MouseScrolledEvent>(event);
				s_MouseStates[inputMode][STATE_WRITE_INDEX].ScrollX = mouseEvent.DeltaX;
				s_MouseStates[inputMode][STATE_WRITE_INDEX].ScrollY = mouseEvent.DeltaY;

				return true;
			}
		}

		return false;
	}

	uint8 Input::ConvertInputModeUINT8(InputMode inputMode)
	{
		return static_cast<uint8>(inputMode);
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
		result = result && EventQueue::RegisterEventHandler<WindowFocusChangedEvent>(eventHandler);

		// Default input mode is game
		PushInputMode(InputMode::GAME);

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
		result = result && EventQueue::UnregisterEventHandler<WindowFocusChangedEvent>(eventHandler);
		return result;
	}

	void Input::Tick()
	{
		std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
		std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);

		uint8 inputMode = ConvertInputModeUINT8(s_InputModeStack.top());

		s_KeyboardStates[inputMode][0] = s_KeyboardStates[inputMode][1];
		s_MouseStates[inputMode][0] = s_MouseStates[inputMode][1];
	}

	void Input::Disable()
	{
		std::scoped_lock<SpinLock> keyboardLock(s_WriteBufferLockKeyboard);
		std::scoped_lock<SpinLock> mouseLock(s_WriteBufferLockMouse);
		s_InputEnabled = false;

		uint8 inputMode = ConvertInputModeUINT8(s_InputModeStack.top());

		std::fill_n(s_KeyboardStates[inputMode][STATE_WRITE_INDEX].KeyStates, EKey::KEY_COUNT, false);
		std::fill_n(s_MouseStates[inputMode][STATE_WRITE_INDEX].ButtonStates, EMouseButton::MOUSE_BUTTON_COUNT, false);
	}
}
