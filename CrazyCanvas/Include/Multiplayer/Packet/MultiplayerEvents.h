#pragma once

#include "Application/API/Events/Event.h"
#include "Networking/API/IClient.h"

struct IPacketReceivedEvent : public LambdaEngine::Event
{
public:
	virtual ~IPacketReceivedEvent() = default;

public:
	virtual void* GetRawData() = 0;
	virtual uint16 GetSize() = 0;
	virtual const LambdaEngine::ComponentType* GetComponentType() = 0;
};

template<class T>
struct PacketReceivedEvent : public IPacketReceivedEvent
{
	friend class PacketType;

public:
	DECLARE_EVENT_TYPE(PacketReceivedEvent);

public:
	PacketReceivedEvent(const LambdaEngine::ComponentType* pCompType) :
		IPacketReceivedEvent(),
		pClient(nullptr),
		pComponentType(pCompType)
	{

	}

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PacketReceivedEvent<T>");
	}

private:
	virtual void* GetRawData() override
	{
		return &Packet;
	}

	virtual uint16 GetSize() override
	{
		return sizeof(T);
	}

	virtual const LambdaEngine::ComponentType* GetComponentType()
	{
		return pComponentType;
	}

public:
	LambdaEngine::IClient* pClient;
	T Packet;

private:
	const LambdaEngine::ComponentType* pComponentType;
};