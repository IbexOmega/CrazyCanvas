#pragma once

#include "Application/API/Events/Event.h"
#include "Networking/API/IClient.h"
#include "ECS/ComponentType.h"

struct IPacketReceivedEvent : public LambdaEngine::Event
{
public:
	virtual ~IPacketReceivedEvent() = default;

public:
	virtual void* Populate(LambdaEngine::IClient* pClient) = 0;
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
	PacketReceivedEvent(const LambdaEngine::ComponentType* pComponentType) :
		IPacketReceivedEvent(),
		pClient(nullptr),
		m_pComponentType(pComponentType)
	{
	}

	virtual LambdaEngine::String ToString() const override
	{
		return LambdaEngine::String("PacketReceivedEvent<T>");
	}

private:
	virtual void* Populate(LambdaEngine::IClient* pNewClient) override
	{
		pClient = pNewClient;
		return &Packet;
	}

	virtual uint16 GetSize() override
	{
		return sizeof(T);
	}

	virtual const LambdaEngine::ComponentType* GetComponentType()
	{
		return m_pComponentType;
	}

public:
	LambdaEngine::IClient* pClient;
	T Packet;

private:
	const LambdaEngine::ComponentType* m_pComponentType;
};