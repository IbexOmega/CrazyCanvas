#include "Multiplayer/PacketDecoderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"
#include "ECS/ComponentArray.h"

#include "ECS/Components/Multiplayer/PacketPlayerActionResponseComponent.h"

#include "Game/Multiplayer/MultiplayerUtils.h"


using namespace LambdaEngine;

PacketDecoderSystem::PacketDecoderSystem()
{
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PacketDecoderSystem::OnPacketReceived);
}

PacketDecoderSystem::~PacketDecoderSystem()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent>(this, &PacketDecoderSystem::OnPacketReceived);
}

void PacketDecoderSystem::Init()
{
	SystemRegistration systemReg = {};
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
	{
		RegisterPacketType<PlayerAction>()
	};
	systemReg.Phase = 0;

	RegisterSystem(systemReg);
}

void PacketDecoderSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	ECSCore* pECS = ECSCore::GetInstance();

	for (auto& pair : m_Entities)
	{
		IComponentArray* pComponents = pECS->GetComponentArray(pair.first);
		for (Entity entity : pair.second)
		{
			void* pComponent = pComponents->GetRawData(entity);
			IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
			pPacketComponent->ClearPacketsReceived();
		}
	}
}

bool PacketDecoderSystem::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();

	event.pPacket->ResetReadHead();

	if (event.Type == NetworkSegment::TYPE_PLAYER_ACTION)
	{
		Entity entity = MultiplayerUtils::GetEntityPlayer(event.pClient);

		if (entity != UINT32_MAX)
		{
			PacketPlayerActionComponent::Packet packet = {};
			event.pPacket->Read(&packet);

			bool isFirstPacketOfTick = false;
			PacketPlayerActionComponent& pPacketComponents = pECS->GetComponent<PacketPlayerActionComponent>(entity, isFirstPacketOfTick);

			pPacketComponents.Packets.PushBack(packet);
		}
	}
	else if (event.Type == NetworkSegment::TYPE_PLAYER_ACTION_RESPONSE)
	{
		BinaryDecoder decoder(event.pPacket);
		Entity entity = MultiplayerUtils::GetEntity(decoder.ReadInt32());

		if (entity != UINT32_MAX)
		{
			PacketPlayerActionResponseComponent::Packet packet = {};
			event.pPacket->Read(&packet);

			bool isFirstPacketOfTick = false;
			PacketPlayerActionResponseComponent& pPacketComponents = pECS->GetComponent<PacketPlayerActionResponseComponent>(entity, isFirstPacketOfTick);

			pPacketComponents.Packets.PushBack(packet);
		}
	}

	return false;
}