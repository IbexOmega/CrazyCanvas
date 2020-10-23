#include "Multiplayer/PacketDecoderSystem.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Networking/NetworkComponent.h"

#include "ECS/ECSCore.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Client/ClientSystem.h"

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

	ComponentArray<NetworkComponent>* pNetworkComponents = pECS->GetComponentArray<NetworkComponent>();

	for (auto& pair : m_ComponentTypeToEntities)
	{
		IComponentArray* pComponents = pECS->GetComponentArray(pair.first);
		for (Entity entity : pair.second)
		{
			const NetworkComponent& networkComponent = pNetworkComponents->GetData(entity);

			void* pComponent = pComponents->GetRawData(entity);
			IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
			pPacketComponent->ClearPacketsReceived();

			//TODO: Make a better implemention between Server and Client
			while (pPacketComponent->GetPacketsToSendCount() > 0)
			{
				if (MultiplayerUtils::IsServer())
				{
					ServerBase* pServer = ServerSystem::GetInstance().GetServer();
					const ClientMap& clients = pServer->GetClients();
					if (!clients.empty())
					{
						ClientRemoteBase* pClient = clients.begin()->second;
						NetworkSegment* pSegment = pClient->GetFreePacket(pPacketComponent->GetPacketType());
						pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
						pClient->SendReliableBroadcast(pSegment);
					}
				}
				else
				{
					ClientBase* pClient = ClientSystem::GetInstance().GetClient();
					NetworkSegment* pSegment = pClient->GetFreePacket(pPacketComponent->GetPacketType());
					pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
					pClient->SendReliable(pSegment);
				}
			}
		}
	}
}

bool PacketDecoderSystem::OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();

	event.pPacket->ResetReadHead();
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
	event.pPacket->Read(packetData, pPacketComponent->GetSize());

	return true;
}