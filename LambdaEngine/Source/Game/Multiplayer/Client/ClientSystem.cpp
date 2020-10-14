#include "Engine/EngineLoop.h"

#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Networking/NetworkPositionComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Physics/Collision.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Input/API/Input.h"

#include "Resources/Material.h"
#include "Resources/ResourceManager.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientUtilsImpl.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem() :
		m_pClient(nullptr),
		m_CharacterControllerSystem(),
		m_NetworkPositionSystem(),
		m_PlayerSystem()
	{

	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
		MultiplayerUtils::Release();
	}

	void ClientSystem::Init()
	{
		MultiplayerUtils::Init(false);

		ClientDesc clientDesc			= {};
		clientDesc.PoolSize				= 1024;
		clientDesc.MaxRetries			= 10;
		clientDesc.ResendRTTMultiplier	= 5.0f;
		clientDesc.Handler				= this;
		clientDesc.Protocol				= EProtocol::UDP;
		clientDesc.PingInterval			= Timestamp::Seconds(1);
		clientDesc.PingTimeout			= Timestamp::Seconds(10);
		clientDesc.UsePingSystem		= true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);

		m_CharacterControllerSystem.Init();
		m_NetworkPositionSystem.Init();
		m_PlayerSystem.Init();
	}

	bool ClientSystem::Connect(IPAddress* pAddress)
	{
		if (pAddress == IPAddress::LOOPBACK)
		{
			MultiplayerUtils::s_IsSinglePlayer = true;

			NetworkSegment* pPacket = m_pClient->GetFreePacket(NetworkSegment::TYPE_ENTITY_CREATE);
			BinaryEncoder encoder3(pPacket);
			encoder3.WriteBool(true);
			encoder3.WriteInt32(0);
			encoder3.WriteVec3(glm::vec3(0, 2, 0));
			OnPacketReceived(m_pClient, pPacket);
			m_pClient->ReturnPacket(pPacket);

			return true;
		}

		MultiplayerUtils::s_IsSinglePlayer = false;

		if (!m_pClient->Connect(IPEndPoint(pAddress, 4444)))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		m_PlayerSystem.FixedTickMainThread(deltaTime, m_pClient);
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		NetworkDebugger::RenderStatistics(m_pClient);
		m_PlayerSystem.TickMainThread(deltaTime, m_pClient);
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnConnected(IClient* pClient)
	{
		ClientConnectedEvent event = {};
		event.pClient = pClient;

		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{
		ClientDisconnectedEvent event = {};
		event.pClient = pClient;

		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		PacketReceivedEvent event = {};
		event.pClient = pClient;
		event.pPacket = pPacket;
		event.Type = pPacket->GetType();

		EventQueue::SendEventImmediate(event);

		if (event.Type == NetworkSegment::TYPE_ENTITY_CREATE)
		{
			BinaryDecoder decoder(pPacket);
			bool isMySelf = decoder.ReadBool();
			int32 networkUID = decoder.ReadInt32();

			if (isMySelf)
				m_PlayerSystem.m_NetworkUID = networkUID;
		}
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
	}

	void ClientSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ClientSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}