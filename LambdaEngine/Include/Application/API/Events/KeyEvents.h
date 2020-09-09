#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"
#include "Input/API/InputState.h"

namespace LambdaEngine
{
	/*
	* Base for KeyEvents
	*/
	struct KeyEvent : public Event
	{
	public:
		KeyEvent(EKey key)
			: Event(EVENT_FLAG_KEYBOARD)
			, Key(key)
		{
		}

		DECLARE_EVENT_TYPE(KeyEvent);

	public:
		EKey Key;
	};

	/*
	* KeyPressedEvent
	*/
	struct KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(EKey key, ModifierKeyState modiferState, bool isRepeat)
			: KeyEvent(key)
			, ModiferState(modiferState)
			, IsRepeat(isRepeat)
		{
		}

		DECLARE_EVENT_TYPE(KeyPressedEvent);

		virtual String ToString() const
		{
			return String("KeyDown=") + KeyToString(Key);
		}

	public:
		ModifierKeyState ModiferState;
		bool IsRepeat;
	};

	/*
	* KeyReleasedEvent
	*/
	struct KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(EKey key)
			: KeyEvent(key)
		{
		}

		DECLARE_EVENT_TYPE(KeyReleasedEvent);

		virtual String ToString() const
		{
			return String("KeyUp=") + KeyToString(Key);
		}
	};

	/*
	* KeyTypedEvent
	*/
	struct KeyTypedEvent : public Event
	{
	public:
		KeyTypedEvent(uint32 character)
			: Event(EVENT_FLAG_KEYBOARD)
			, Character(Character)
		{
		}

		DECLARE_EVENT_TYPE(KeyTypedEvent);

		virtual String ToString() const
		{
			return String("KeyTypedEvent=") + GetPrintableChar();
		}

		char GetPrintableChar() const
		{
			return static_cast<char>(Character);
		}

	public:
		uint32 Character;
	};
}