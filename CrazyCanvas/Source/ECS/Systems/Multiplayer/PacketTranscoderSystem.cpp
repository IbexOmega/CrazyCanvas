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

PacketTranscoderSystem PacketTranscoderSystem::s_Instance;

void PacketTranscoderSystem::Init()
{
	EventQueue::RegisterEventHandler<NetworkSegmentReceivedEvent>(this, &PacketTranscoderSystem::OnPacketReceived);

	SystemRegistration systemReg;

	const PacketTypeMap& packetTypeMap = PacketType::GetPacketTypeMap();
	systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.Reserve((uint32)packetTypeMap.size());

	for (auto pair : packetTypeMap)
	{
		IPacketReceivedEvent* pEvent = pair.second;
		const ComponentType* pComponentType = pEvent->GetComponentType();
		if (pComponentType)
		{
			EntitySubscriptionRegistration subscription;
			subscription.pSubscriber = &m_ComponentTypeToEntities[pComponentType];
			subscription.ComponentAccesses =
			{
				{NDA, NetworkComponent::Type()},
				{RW, pComponentType},
			};
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations.PushBack(subscription);
		}
	}

	systemReg.Phase = 0;

	RegisterSystem(TYPE_NAME(PacketTranscoderSystem), systemReg);
}

void PacketTranscoderSystem::Release()
{
	EventQueue::UnregisterEventHandler<NetworkSegmentReceivedEvent>(this, &PacketTranscoderSystem::OnPacketReceived);
}

void PacketTranscoderSystem::FixedTickMainThreadClient(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<NetworkComponent>* pNetworkComponents = pECS->GetComponentArray<NetworkComponent>();

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

			const uint16 packetType = pPacketComponent->GetPacketType();
			VALIDATE_MSG(packetType != 0, "Packet type not registered, have you forgotten to register your package?");

			while (pPacketComponent->GetPacketsToSendCount() > 0)
			{
				NetworkSegment* pSegment = pClient->GetFreePacket(packetType);
				pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
				pClient->SendReliable(pSegment);
			}
		}
	}
}

void PacketTranscoderSystem::FixedTickMainThreadServer(LambdaEngine::Timestamp deltaTime)
{
	UNREFERENCED_VARIABLE(deltaTime);

	ECSCore* pECS = ECSCore::GetInstance();

	ComponentArray<NetworkComponent>* pNetworkComponents = pECS->GetComponentArray<NetworkComponent>();

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

			const uint16 packetType = pPacketComponent->GetPacketType();
			VALIDATE_MSG(packetType != 0, "Packet type not registered, have you forgotten to register your package?");

			if (pClient)
			{
				while (pPacketComponent->GetPacketsToSendCount() > 0)
				{
					NetworkSegment* pSegment = pClient->GetFreePacket(packetType);
					pPacketComponent->WriteSegment(pSegment, networkComponent.NetworkUID);
					pClient->SendReliableBroadcast(pSegment);
				}
			}
		}
	}
}

bool PacketTranscoderSystem::OnPacketReceived(const LambdaEngine::NetworkSegmentReceivedEvent& event)
{
	ECSCore* pECS = ECSCore::GetInstance();
	NetworkSegment* pSegment = event.pPacket;
	uint16 packetType = event.Type;

	IPacketReceivedEvent* pEvent = PacketType::GetPacketReceivedEventPointer(packetType);
	if (!pEvent)
		return false;

	uint16 packetSize = pEvent->GetSize();
	if (packetSize != pSegment->GetBufferSize())
		return true;

	pSegment->ResetReadHead();
	pSegment->Read(pEvent->Populate(event.pClient), packetSize);
	EventQueue::SendEventImmediate(*pEvent);

	const ComponentType* pComponentType = pEvent->GetComponentType();

	if (!pComponentType)
		return true;

	const Packet* pPacket = (const Packet*)event.pPacket->GetBuffer();
	Entity entity = MultiplayerUtils::GetEntity(pPacket->NetworkUID);
	if (entity == UINT32_MAX)
		return true;

	if (!MultiplayerUtils::HasWriteAccessToEntity(entity))
		return true;

	IComponentArray* pComponents = pECS->GetComponentArray(pComponentType);
	void* pComponent = pComponents->GetRawData(entity);
	IPacketComponent* pPacketComponent = static_cast<IPacketComponent*>(pComponent);
	void* packetData = pPacketComponent->AddPacketReceivedBegin();
	pSegment->ResetReadHead();
	pSegment->Read(packetData, packetSize);
	pPacketComponent->AddPacketReceivedEnd();

	return true;
}