#pragma once

#include "Event.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkSegment.h"

namespace LambdaEngine
{
	struct ClientConnectedEvent : public Event
	{
	public:
		inline ClientConnectedEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(ClientConnectedEvent);

		virtual String ToString() const override
		{
			return String("ClientConnectedEvent");
		}

	public:
		IClient* pClient;
	};

	struct ClientDisconnectedEvent : public Event
	{
	public:
		inline ClientDisconnectedEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(ClientDisconnectedEvent);

		virtual String ToString() const override
		{
			return String("ClientDisconnectedEvent");
		}

	public:
		IClient* pClient;
	};

	struct PacketReceivedEvent : public Event
	{
	public:
		inline PacketReceivedEvent()
			: Event()
		{
		}

		DECLARE_EVENT_TYPE(PacketReceivedEvent);

		virtual String ToString() const override
		{
			return String("PacketReceivedEvent=" + pPacket->ToString());
		}

	public:
		IClient* pClient;
		NetworkSegment* pPacket;
		uint16 Type;
	};
}