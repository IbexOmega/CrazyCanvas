#pragma once

#include "Event.h"

#include "Networking/API/IClient.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryDecoder.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/ServerBase.h"

#define DECLARE_CONNECTION_EVENT_TYPE(Type) \
		DECLARE_EVENT_TYPE(Type); \
		public: \
			virtual String ToString() const override \
			{ \
				return String(#Type); \
			} \

namespace LambdaEngine
{
	struct ClientConnectionEventBase : public Event
	{
	protected:
		inline ClientConnectionEventBase(IClient* pIClient)
			: Event(),
			pClient(pIClient)
		{

		}

	public:
		IClient* pClient;
	};

	struct ClientConnectingEvent : public ClientConnectionEventBase
	{
	public:
		inline ClientConnectingEvent(IClient* pIClient)
			: ClientConnectionEventBase(pIClient)
		{
		}

		DECLARE_CONNECTION_EVENT_TYPE(ClientConnectingEvent);
	};

	struct ClientConnectedEvent : public ClientConnectionEventBase
	{
	public:
		inline ClientConnectedEvent(IClient* pIClient)
			: ClientConnectionEventBase(pIClient)
		{
		}

		DECLARE_CONNECTION_EVENT_TYPE(ClientConnectedEvent);
	};

	struct ClientDisconnectingEvent : public ClientConnectionEventBase
	{
	public:
		inline ClientDisconnectingEvent(IClient* pIClient)
			: ClientConnectionEventBase(pIClient)
		{
		}

		DECLARE_CONNECTION_EVENT_TYPE(ClientDisconnectingEvent);
	};

	struct ClientDisconnectedEvent : public ClientConnectionEventBase
	{
	public:
		inline ClientDisconnectedEvent(IClient* pIClient)
			: ClientConnectionEventBase(pIClient)
		{
		}

		DECLARE_CONNECTION_EVENT_TYPE(ClientDisconnectedEvent);
	};

	struct ServerFullEvent : public ClientConnectionEventBase
	{
	public:
		inline ServerFullEvent(IClient* pIClient)
			: ClientConnectionEventBase(pIClient)
		{
		}

		DECLARE_CONNECTION_EVENT_TYPE(ServerFullEvent);
	};

	struct NetworkSegmentReceivedEvent : public Event
	{
	public:
		inline NetworkSegmentReceivedEvent(IClient* pIClient, NetworkSegment* pNetworkSegment)
			: Event(),
			pClient(pIClient),
			pPacket(pNetworkSegment),
			Type(pNetworkSegment->GetType())
		{
		}

		DECLARE_EVENT_TYPE(NetworkSegmentReceivedEvent);

		virtual String ToString() const override
		{
			return String("NetworkSegmentReceivedEvent=" + pPacket->ToString());
		}

	public:
		IClient* pClient;
		NetworkSegment* pPacket;
		uint16 Type;
	};

	struct ServerDiscoveredEvent : public Event
	{
	public:
		inline ServerDiscoveredEvent(BinaryDecoder* pBinaryDecoder, const IPEndPoint* pIPEndPoint, uint64 serverUID, Timestamp ping)
			: Event(),
			pDecoder(pBinaryDecoder),
			pEndPoint(pIPEndPoint),
			ServerUID(serverUID),
			Ping(ping)
		{
		}

		DECLARE_EVENT_TYPE(ServerDiscoveredEvent);

		virtual String ToString() const override
		{
			return String("ServerDiscoveredEvent=" + pEndPoint->ToString());
		}

	public:
		BinaryDecoder* pDecoder;
		const IPEndPoint* pEndPoint;
		uint64 ServerUID;
		Timestamp Ping;
	};

	struct ServerDiscoveryPreTransmitEvent : public Event
	{
	public:
		inline ServerDiscoveryPreTransmitEvent(BinaryEncoder* pBinaryEncoder, ServerBase* pServerBase)
			: Event(),
			pEncoder(pBinaryEncoder),
			pServer(pServerBase)
		{

		}

		DECLARE_EVENT_TYPE(ServerDiscoveryPreTransmitEvent);

		virtual String ToString() const override
		{
			return String("ServerDiscoveryPreTransmitEvent");
		}

	public:
		BinaryEncoder* pEncoder;
		ServerBase* pServer;
	};
}