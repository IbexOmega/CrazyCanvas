#pragma once

#include "ECS/System.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/ComponentType.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Containers/THashTable.h"

class PacketTranscoderSystem : public LambdaEngine::System
{
public:
	PacketTranscoderSystem();
	~PacketTranscoderSystem();

	void Init();

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final {};

private:
	LambdaEngine::THashTable<const LambdaEngine::ComponentType*, LambdaEngine::IDVector> m_ComponentTypeToEntities;
};
