#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Networking/API/NetworkDebugger.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientUtilsImpl.h"

#include "Application/API/Events/EventQueue.h"

#include "Engine/EngineConfig.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem(const String& name) :
		m_pClient(nullptr),
		m_Name(name),
		m_DebuggingWindow(false)
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
		clientDesc.UsePingSystem		= false;

		m_pClient = NetworkUtils::CreateClient(clientDesc);

		NetworkDiscovery::EnableClient(m_Name, this);

		ConsoleCommand netStatsCmd;
		netStatsCmd.Init("show_net_stats", m_DebuggingWindow);
		netStatsCmd.AddArg(Arg::EType::BOOL);
		netStatsCmd.AddDescription("Activate/Deactivate Network debugging window.\n\t'show_net_stats true'");
		GameConsole::Get().BindCommand(netStatsCmd, [&, this](GameConsole::CallbackInput& input)->void {
			m_DebuggingWindow = input.Arguments.GetFront().Value.Boolean;
		});

		EventQueue::RegisterEventHandler<ClientDisconnectedEvent>(this, &ClientSystem::OnDisconnectedEvent);
	}

	ClientSystem::~ClientSystem()
	{
		m_pClient->Release();
		MultiplayerUtils::Release();
	}

	bool ClientSystem::Connect(IPAddress* pAddress)
	{
		NetworkDiscovery::DisableClient();

		MultiplayerUtils::s_IsSinglePlayer = false;

		if (!m_pClient->Connect(IPEndPoint(pAddress, (uint16)EngineConfig::GetIntProperty("NetworkPort"))))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}
		return true;
	}

	ClientBase* ClientSystem::GetClient()
	{
		return m_pClient;
	}

	void ClientSystem::TickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		if(m_DebuggingWindow)
			NetworkDebugger::RenderStatistics(m_pClient);
	}

	void ClientSystem::OnConnecting(IClient* pClient)
	{
		ClientConnectingEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientSystem::OnConnected(IClient* pClient)
	{
		ClientConnectedEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientSystem::OnDisconnecting(IClient* pClient)
	{
		ClientDisconnectingEvent event(pClient);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnDisconnected(IClient* pClient)
	{
		ClientDisconnectedEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	bool ClientSystem::OnDisconnectedEvent(const ClientDisconnectedEvent& event)
	{
		UNREFERENCED_VARIABLE(event);

		NetworkDiscovery::EnableClient(m_Name, this);
		return false;
	}

	void ClientSystem::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
	{
		NetworkSegmentReceivedEvent event(pClient, pPacket);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnClientReleased(IClient* pClient)
	{
		UNREFERENCED_VARIABLE(pClient);
	}

	void ClientSystem::OnServerFull(IClient* pClient)
	{
		ServerFullEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientSystem::OnServerFound(BinaryDecoder& decoder, const IPEndPoint& endPoint, uint64 serverUID)
	{
		ServerDiscoveredEvent event(&decoder, &endPoint, serverUID);
		EventQueue::SendEventImmediate(event);
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