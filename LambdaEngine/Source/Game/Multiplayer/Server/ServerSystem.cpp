#include "Game/Multiplayer/Server/ServerSystem.h"
#include "Game/Multiplayer/Server/ClientRemoteSystem.h"

#include "ECS/ECSCore.h"

#include "Networking/API/NetworkDebugger.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

#include "Engine/EngineConfig.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	ServerSystem* ServerSystem::s_pInstance = nullptr;

	ServerSystem::ServerSystem(const String& name) :
		m_NetworkEntities(),
		m_pServer(nullptr),
		m_Name(name)
	{
		MultiplayerUtils::Init(true);

		const String& protocol = EngineConfig::GetStringProperty(CONFIG_OPTION_NETWORK_PROTOCOL);

		ServerDesc desc = {};
		desc.Handler				= this;
		desc.MaxRetries				= 25;
		desc.ResendRTTMultiplier	= 10.0f;
		desc.MaxClients				= 10;
		desc.PoolSize				= 8192;
		desc.Protocol				= EProtocolParser::FromString(protocol);
		desc.PingInterval			= Timestamp::Seconds(1);
		desc.PingTimeout			= Timestamp::Seconds(10);
		desc.UsePingSystem			= EngineConfig::GetBoolProperty(CONFIG_OPTION_NETWORK_PING_SYSTEM);

		m_pServer = NetworkUtils::CreateServer(desc);
		//((ServerUDP*)m_pServer)->SetSimulateReceivingPacketLoss(0.1f);
	}

	ServerSystem::~ServerSystem()
	{
		m_pServer->Release();
		MultiplayerUtils::Release();
	}

	bool ServerSystem::Start()
	{
		uint16 port = (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
		//NetworkDiscovery::EnableServer(m_Name, port, this);
		return m_pServer->Start(IPEndPoint(IPAddress::ANY, port));
	}

	void ServerSystem::Stop()
	{
		NetworkDiscovery::DisableServer();
		m_pServer->Stop("ServerSystem->Stop()");
	}

	void ServerSystem::TickMainThread(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
		NetworkDebugger::RenderStatistics(m_pServer);
	}

	ServerBase* ServerSystem::GetServer()
	{
		return m_pServer;
	}

	IClientRemoteHandler* ServerSystem::CreateClientHandler()
	{
		return DBG_NEW ClientRemoteSystem();
	}

	void ServerSystem::OnNetworkDiscoveryPreTransmit(BinaryEncoder& encoder)
	{
		ServerDiscoveryPreTransmitEvent event(&encoder, m_pServer);
		EventQueue::SendEventImmediate(event);
	}

	void ServerSystem::StaticTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->TickMainThread(deltaTime);
	}

	void ServerSystem::StaticRelease()
	{
		SAFEDELETE(s_pInstance);
	}
}