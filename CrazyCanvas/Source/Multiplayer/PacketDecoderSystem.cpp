#include "Multiplayer/PacketDecoderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"
#include "ECS/ECSCore.h"

#include "ECS/Components/Multiplayer/PacketPlayerActionComponent.h"
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
		{
			.pSubscriber = &m_PlayerEntities,
			.ComponentAccesses =
			{
				{NDA, NetworkComponent::Type()},
				{RW, PacketPlayerActionComponent::Type()},
			}
		}
	};
	systemReg.Phase = 0;

	RegisterSystem(systemReg);
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

			if (isFirstPacketOfTick)
				pPacketComponents.Packets.Clear();

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

			if (isFirstPacketOfTick)
			{
				LOG_WARNING("Clear");
				pPacketComponents.Packets.Clear();
			}

			LOG_WARNING("Adding %d", packet.SimulationTick);
			pPacketComponents.Packets.PushBack(packet);
		}
	}

	return false;
}
