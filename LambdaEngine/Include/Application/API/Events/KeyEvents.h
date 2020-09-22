#pragma once
#include "Event.h"

#include "Input/API/InputCodes.h"
#include "Input/API/InputState.h"

namespace LambdaEngine
{
	/*
	* KeyPressedEvent
	*/
	struct KeyPressedEvent : public Event
	{
	public:
		inline KeyPressedEvent(EKey key, ModifierKeyState modiferState, bool isRepeat)
			: Event()
			, Key(key)
			, ModiferState(modiferState)
			, IsRepeat(isRepeat)
		{
		}

		DECLARE_EVENT_TYPE(KeyPressedEvent);

		virtual String ToString() const override
		{
			return String("KeyPressedEvent=") + KeyToString(Key);
		}

	public:
		EKey Key;
		ModifierKeyState ModiferState;
		bool IsRepeat;
	};

	/*
	* KeyReleasedEvent
	*/
	struct KeyReleasedEvent : public Event
	{
	public:
		inline KeyReleasedEvent(EKey key, ModifierKeyState modiferState)
			: Event()
			, Key(key)
			, ModiferState(modiferState)
		{
		}

		DECLARE_EVENT_TYPE(KeyReleasedEvent);

		virtual String ToString() const override
		{
			return String("KeyReleasedEvent=") + KeyToString(Key);
		}

	public:
		EKey Key;
		ModifierKeyState ModiferState;
	};

	/*
	* KeyTypedEvent
	*/
	struct KeyTypedEvent : public Event
	{
	public:
		inline KeyTypedEvent(uint32 character)
			: Event()
			, Character(character)
		{
		}

		DECLARE_EVENT_TYPE(KeyTypedEvent);

		virtual String ToString() const override
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