#include "Chat/ChatManager.h"

struct ChatEvent : public LambdaEngine::Event
{
public:
	inline ChatEvent(const ChatMessage& message)
		: Event(),
		Message(message)
	{
	}

	DECLARE_EVENT_TYPE(ChatEvent);

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("ChatEvent=" + Message.Name);
	}

	bool IsSystemMessage() const
	{
		return Message.UID == UINT64_MAX;
	}

public:
	const ChatMessage& Message;
};