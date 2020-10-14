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

#include "Engine/EngineConfig.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem(const String& name) :
		m_pClient(nullptr),
		m_CharacterControllerSystem(),
		m_NetworkPositionSystem(),
		m_PlayerSystem(),
		m_Name(name),
		m_DebuggingWindow(false)
	{
		MultiplayerUtils::Init(false);

		ClientDesc clientDesc			= {};
		clientDesc.PoolSize				= 1024;
		clientDesc.MaxRetries			= 10;
		clientDesc.ResendRTTMultiplier	= 5.0F;
		clientDesc.Handler				= this;
		clientDesc.Protocol				= EProtocol::UDP;
		clientDesc.PingInterval			= Timestamp::Seconds(1);
		clientDesc.PingTimeout			= Timestamp::Seconds(10);
		clientDesc.UsePingSystem		= true;

		m_pClient = NetworkUtils::CreateClient(clientDesc);

		m_CharacterControllerSystem.Init();
		m_NetworkPositionSystem.Init();
		m_PlayerSystem.Init();

		NetworkDiscovery::EnableClient(m_Name, this);

		ConsoleCommand netStatsCmd;
		netStatsCmd.Init("show_net_stats", m_DebuggingWindow);
		netStatsCmd.AddArg(Arg::EType::BOOL);
		netStatsCmd.AddDescription("Activate/Deactivate Network debugging window.\n\t'show_net_stats true'");
		GameConsole::Get().BindCommand(netStatsCmd, [&, this](GameConsole::CallbackInput& input)->void {
			m_DebuggingWindow = input.Arguments.GetFront().Value.Boolean;
		});
	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
		MultiplayerUtils::Release();
	}

	bool ClientSystem::Connect(IPAddress* pAddress)
	{
		NetworkDiscovery::DisableClient();

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

		if (!m_pClient->Connect(IPEndPoint(pAddress, (uint16)EngineConfig::GetIntProperty("NetworkPort"))))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	void ClientSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		m_PlayerSystem.FixedTickMainThread(deltaTime, m_pClient);
		m_CharacterControllerSystem.FixedTickMainThread(deltaTime);
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		if(m_DebuggingWindow)
			NetworkDebugger::RenderStatistics(m_pClient);

		m_PlayerSystem.TickMainThread(deltaTime, m_pClient);
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{
		ClientConnectingEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnConnected(IClient* pClient)
	{
		ClientConnectedEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{
		ClientDisconnectingEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{
		ClientDisconnectedEvent event(pClient);
		EventQueue::SendEventImmediate(event);

		NetworkDiscovery::EnableClient(m_Name, this);
	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		PacketReceivedEvent event(pClient, pPacket);
		EventQueue::SendEventImmediate(event);

		if (pPacket->GetType() == NetworkSegment::TYPE_ENTITY_CREATE)
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
		ServerFullEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnServerFound(BinaryDecoder& decoder, const IPEndPoint& endPoint, uint64 serverUID)
	{
		ServerDiscoveredEvent event(&decoder, &endPoint, serverUID);
		EventQueue::SendEventImmediate(event);
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