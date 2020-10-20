#pragma once

#include "ECS/System.h"
#include "ECS/EntitySubscriber.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Containers/THashTable.h"

class PacketDecoderSystem : public LambdaEngine::System
{
public:
	PacketDecoderSystem();
	~PacketDecoderSystem();

	void Init();

	template<typename PacketType>
	EntitySubscriptionRegistration RegisterPacketType()
	{
		const ComponentType* type = PacketComponent<PacketType>::Type();

		EntitySubscriptionRegistration subscription;
		subscription.pSubscriber = &m_Entities[type];
		subscription.ComponentAccesses =
		{
			{NDA, NetworkComponent::Type()},
			{RW, type},
		};
		return subscription;
	}

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final {};

private:
	LambdaEngine::IDVector m_PlayerEntities;
	LambdaEngine::IDVector m_PlayerEntities2;

	LambdaEngine::THashTable<const ComponentType*, LambdaEngine::IDVector> m_Entities;
};