#include "Game/Multiplayer/Client/ClientSystem.h"

#include "Networking/API/NetworkDebugger.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/Multiplayer/Client/ClientUtilsImpl.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/GameConsole.h"

#include "Engine/EngineConfig.h"

namespace LambdaEngine
{
	ClientSystem* ClientSystem::s_pInstance = nullptr;

	ClientSystem::ClientSystem(const String& name) :
		m_pClient(nullptr),
		m_Name(name),
		m_DebuggingWindow(false)
	{
		MultiplayerUtils::Init(false);

		const String& protocol = EngineConfig::GetStringProperty(CONFIG_OPTION_NETWORK_PROTOCOL);

		ClientDesc clientDesc			= {};
		clientDesc.PoolSize				= 8192;
		clientDesc.MaxRetries			= 25;
		clientDesc.ResendRTTMultiplier	= 5.0f;
		clientDesc.Handler				= this;
		clientDesc.Protocol				= EProtocolParser::FromString(protocol);
		clientDesc.PingInterval			= Timestamp::Seconds(1);
		clientDesc.PingTimeout			= Timestamp::Seconds(10);
		clientDesc.UsePingSystem		= EngineConfig::GetBoolProperty(CONFIG_OPTION_NETWORK_PING_SYSTEM);

		m_pClient = NetworkUtils::CreateClient(clientDesc);

		NetworkDiscovery::EnableClient(m_Name, this);

		ConsoleCommand netStatsCmd;
		netStatsCmd.Init("show_net_stats", true);
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

	bool ClientSystem::Connect(const IPEndPoint& endPoint)
	{
		MultiplayerUtils::s_IsSinglePlayer = false;

		if (!m_pClient->Connect(endPoint))
		{
			LOG_ERROR("Failed to connect!");
			return false;
		}

		NetworkDiscovery::DisableClient();
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

	void ClientSystem::OnDisconnecting(IClient* pClient, const String& reason)
	{
		ClientDisconnectingEvent event(pClient, reason);
		EventQueue::SendEventImmediate(event);
	}

	void ClientSystem::OnDisconnected(IClient* pClient, const String& reason)
	{
		ClientDisconnectedEvent event(pClient, reason);
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

	void ClientSystem::OnServerNotAccepting(IClient* pClient)
	{
		ServerNotAcceptingEvent event(pClient);
		EventQueue::SendEvent(event);
	}

	void ClientSystem::OnServerFound(BinaryDecoder& decoder, const IPEndPoint& endPoint, uint64 serverUID, Timestamp ping, bool isLAN)
	{
		ServerDiscoveredEvent event(&decoder, &endPoint, serverUID, ping, isLAN);
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