#include "Multiplayer/PacketDecoderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Multiplayer/PacketType.h"

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
	SystemRegistration systemReg;

	const PacketTypeMap& packetTypeMap = PacketType::GetPacketTypeMap();

	for (auto pair : packetTypeMap)
	{
		EntitySubscriptionRegistration subscription;
		subscription.pSubscriber = &m_ComponentTypeToEntities[pair.second];
		subscription.ComponentAccesses =
		{
			{NDA, NetworkComponent::Type()},
			{RW, pair.second},
		};
		systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(subscription);
	}

	systemReg.Phase = 0;

	RegisterSystem(systemReg);
}

void PacketDecoderSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	ECSCore* pECS = ECSCore::GetInstance();

	for (auto& pair : m_ComponentTypeToEntities)
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

	if (event.Type == PacketType::PLAYER_ACTION)
	{
		Entity entity = MultiplayerUtils::GetEntityPlayer(event.pClient);

		if (entity != UINT32_MAX)
		{
			const ComponentType* pType = PacketType::GetComponentType(event.Type);
			IComponentArray* pComponents = pECS->GetComponentArray(pType);
			void* pComponent = pComponents->GetRawData(entity);
			IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
			void* packetData = pPacketComponent->AddPacketReceived();
			event.pPacket->Read(packetData, pPacketComponent->GetSize());
		}
	}
	else if (event.Type == PacketType::PLAYER_ACTION_RESPONSE)
	{
		BinaryDecoder decoder(event.pPacket);
		Entity entity = MultiplayerUtils::GetEntity(decoder.ReadInt32());

		if (entity != UINT32_MAX)
		{
			const ComponentType* pType = PacketType::GetComponentType(event.Type);
			IComponentArray* pComponents = pECS->GetComponentArray(pType);
			void* pComponent = pComponents->GetRawData(entity);
			IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
			void* packetData = pPacketComponent->AddPacketReceived();
			event.pPacket->Read(packetData, pPacketComponent->GetSize());
		}
	}

	return false;
}