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
		inline explicit KeyEvent(EKey key, ModifierKeyState modiferState)
			: Event(EVENT_FLAG_KEYBOARD)
			, Key(key)
			, ModiferState(modiferState)
		{
		}

	public:
		EKey Key;
		ModifierKeyState ModiferState;
	};

	/*
	* KeyDownEvent
	*/
	struct KeyDownEvent : public KeyEvent
	{
	public:
		inline explicit KeyDownEvent(EKey key, ModifierKeyState modiferState)
			: KeyEvent(key, modiferState)
		{
		}
	};

	/*
	* KeyUpEvent
	*/
	struct KeyUpEvent : public KeyEvent
	{
	public:
		inline explicit KeyUpEvent(EKey key, ModifierKeyState modiferState)
			: KeyEvent(key, modiferState)
		{
		}
	};

	/*
	* KeyTypedEvent
	*/
	struct KeyTypedEvent : public Event
	{
	public:
		inline explicit KeyTypedEvent(uint32 character)
			: Event(EVENT_FLAG_KEYBOARD)
			, Character(Character)
		{
		}

	public:
		uint32 Character;
	};
}