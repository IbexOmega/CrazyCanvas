#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Multiplayer/Packet/PacketType.h"

using namespace LambdaEngine;

PacketTranscoderSystem::PacketTranscoderSystem()
{
	EventQueue::RegisterEventHandler<PacketReceivedEvent>(this, &PacketTranscoderSystem::OnPacketReceived);
}

PacketTranscoderSystem::~PacketTranscoderSystem()
{
	EventQueue::UnregisterEventHandler<PacketReceivedEvent>(this, &PacketTranscoderSystem::OnPacketReceived);
}

void PacketTranscoderSystem::Init()
{
	SystemRegistration systemReg;

	const PacketTypeMap& packetTypeMap = PacketType::GetPacketTypeMap();
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.Reserve((uint32)packetTypeMap.size());

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

	RegisterSystem(TYPE_NAME(PacketTranscoderSystem), systemReg);
}

void PacketTranscoderSystem::FixedTickMainThread(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<NetworkComponent>* pNetworkComponents = pECS->GetComponentArray<NetworkComponent>();

	if (MultiplayerUtils::IsServer())
	{
		ServerBase* pServer = ServerSystem::GetInstance().GetServer();
		const ClientMap& clients = pServer->GetClients();
		ClientRemoteBase* pClient = nullptr;
		if (!clients.empty())
			pClient = clients.begin()->second;

		for (auto& pair : m_ComponentTypeToEntities)
		{
			IComponentArray* pComponents = pECS->GetComponentArray(pair.first);
			for (Entity entity : pair.second)
			{
				const NetworkComponent& networkComponent = pNetworkComponents->GetData(entity);

				void* pComponent = pComponents->GetRawData(entity);
				IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
				pPacketComponent->ClearPacketsReceived();

				if (pClient)
				{
					while (pPacketComponent->GetPacketsToSendCount() > 0)
					{
						NetworkSegment* pSegment = pClient->GetFreePacket(pPacketComponent->GetPacketType());
						pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
						pClient->SendReliableBroadcast(pSegment);
					}
				}
			}
		}
	}
	else
	{
		ClientBase* pClient = ClientSystem::GetInstance().GetClient();

		for (auto& pair : m_ComponentTypeToEntities)
		{
			IComponentArray* pComponents = pECS->GetComponentArray(pair.first);
			for (Entity entity : pair.second)
			{
				const NetworkComponent& networkComponent = pNetworkComponents->GetData(entity);

				void* pComponent = pComponents->GetRawData(entity);
				IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
				pPacketComponent->ClearPacketsReceived();

				while (pPacketComponent->GetPacketsToSendCount() > 0)
				{
					NetworkSegment* pSegment = pClient->GetFreePacket(pPacketComponent->GetPacketType());
					pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
					pClient->SendReliable(pSegment);
				}
			}
		}
	}
}

bool PacketTranscoderSystem::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();
	NetworkSegment* pSegment = event.pPacket;

	pSegment->ResetReadHead();
	const ComponentType* pComponentType = PacketType::GetComponentType(event.Type);

	if (!pComponentType)
		return false;

	const Packet* pPacket = (const Packet*)event.pPacket->GetBuffer();
	Entity entity = MultiplayerUtils::GetEntity(pPacket->NetworkUID);

	if (entity == UINT32_MAX)
		return true;

	if (!MultiplayerUtils::HasWriteAccessToEntity(entity))
		return true;

	IComponentArray* pComponents = pECS->GetComponentArray(pComponentType);
	void* pComponent = pComponents->GetRawData(entity);
	IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
	void* packetData = pPacketComponent->AddPacketReceived();
	pSegment->Read(packetData, pPacketComponent->GetSize());

	return true;
}