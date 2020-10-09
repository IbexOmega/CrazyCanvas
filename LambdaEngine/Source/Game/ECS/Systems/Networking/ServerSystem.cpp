#include "Game/ECS/Systems/Networking/ServerSystem.h"

#include "Game/ECS/Components/Physics/Transform.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Systems/Networking/ClientRemoteSystem.h"

#include "Networking/API/NetworkDebugger.h"

namespace LambdaEngine
{
	ServerSystem* ServerSystem::s_pInstance = nullptr;

	ServerSystem::ServerSystem() :
		m_NetworkEntities(),
		m_pServer(nullptr)
	{
		ServerDesc desc = {};
		desc.Handler				= this;
		desc.MaxRetries				= 10;
		desc.ResendRTTMultiplier	= 5.0f;
		desc.MaxClients				= 10;
		desc.PoolSize				= 1024;
		desc.Protocol				= EProtocol::UDP;
		desc.PingInterval			= Timestamp::Seconds(1);
		desc.PingTimeout			= Timestamp::Seconds(3);
		desc.UsePingSystem			= true;

		m_pServer = NetworkUtils::CreateServer(desc);
		//((ServerUDP*)m_pServer)->SetSimulateReceivingPacketLoss(0.1f);

		SystemRegistration systemReg = {};
		RegisterSystem(systemReg);
	}

	ServerSystem::~ServerSystem()
	{
		m_pServer->Release();
	}

	bool ServerSystem::Start()
	{
		return m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));
	}

	void ServerSystem::Stop()
	{
		m_pServer->Stop("ServerSystem->Stop()");
	}

	void ServerSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);
	}

	void ServerSystem::FixedTickMainThread(Timestamp deltaTime)
	{
		const ClientMap& pClients = m_pServer->GetClients();
		for (auto& pair : pClients)
		{
			ClientRemoteSystem* pClientSystem = (ClientRemoteSystem*)pair.second->GetHandler();
			pClientSystem->FixedTickMainThread(deltaTime);
		}
	}

	void ServerSystem::TickMainThread(Timestamp deltaTime)
	{
		NetworkDebugger::RenderStatistics(m_pServer);

		const ClientMap& pClients = m_pServer->GetClients();
		for (auto& pair : pClients)
		{
			ClientRemoteSystem* pClientSystem = (ClientRemoteSystem*)pair.second->GetHandler();
			pClientSystem->TickMainThread(deltaTime);
		}
	}

	IClientRemoteHandler* ServerSystem::CreateClientHandler()
	{
		return DBG_NEW ClientRemoteSystem();
	}

	void ServerSystem::StaticFixedTickMainThread(Timestamp deltaTime)
	{
		if (s_pInstance)
			s_pInstance->FixedTickMainThread(deltaTime);
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