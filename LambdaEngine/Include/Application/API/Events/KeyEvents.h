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
		inline KeyEvent(EKey key, ModifierKeyState modiferState)
			: Event(EVENT_FLAG_KEYBOARD)
			, Key(key)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(KeyEvent);

	public:
		EKey Key;
		ModifierKeyState ModiferState;
	};

	/*
	* KeyPressedEvent
	*/
	struct KeyPressedEvent : public KeyEvent
	{
	public:
		inline KeyPressedEvent(EKey key, ModifierKeyState modiferState, bool isRepeat)
			: KeyEvent(key, modiferState)
			, IsRepeat(isRepeat)
		{
		}

		DECLARE_EVENT_TYPE(KeyPressedEvent);

		virtual String ToString() const
		{
			return String("KeyDown=") + KeyToString(Key);
		}

	public:
		bool IsRepeat;
	};

	/*
	* KeyReleasedEvent
	*/
	struct KeyReleasedEvent : public KeyEvent
	{
	public:
		inline KeyReleasedEvent(EKey key, ModifierKeyState modiferState)
			: KeyEvent(key, modiferState)
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
		inline KeyTypedEvent(uint32 character)
			: Event(EVENT_FLAG_KEYBOARD)
			, Character(character)
		{
		}

		DECLARE_EVENT_TYPE(KeyTypedEvent);

		virtual String ToString() const
		{
			return String("KeyTypedEvent=") + GetPrintableChar();
		}

		FORCEINLINE char GetPrintableChar() const
		{
			return static_cast<char>(Character);
		}

	public:
		uint32 Character;
	};
}