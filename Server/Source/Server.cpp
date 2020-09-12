#include "Server.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Threading/API/Thread.h"

#include "Networking/API/NetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkSegment.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

#include "ClientHandler.h"

#include "Math/Random.h"


Server::Server()
{
	using namespace LambdaEngine;
	CommonApplication::Get()->AddEventHandler(this);


	ServerDesc desc = {};
	desc.Handler		= this;
	desc.MaxRetries		= 10;
	desc.MaxClients		= 10;
	desc.PoolSize		= 512;
	desc.Protocol		= EProtocol::UDP;
	desc.PingInterval	= Timestamp::Seconds(1);
	desc.PingTimeout	= Timestamp::Seconds(3);

	m_pServer = NetworkUtils::CreateServer(desc);
	m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));

	//m_pServer->SetSimulateReceivingPacketLoss(0.1f);
}

Server::~Server()
{
	m_pServer->Release();
}

void Server::OnClientConnected(LambdaEngine::IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
}

LambdaEngine::IClientRemoteHandler* Server::CreateClientHandler()
{
	return DBG_NEW ClientHandler();
}

void Server::OnKeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	UNREFERENCED_VARIABLE(key);
	UNREFERENCED_VARIABLE(modifierMask);
	UNREFERENCED_VARIABLE(isRepeat);

	using namespace LambdaEngine;

	if(m_pServer->IsRunning())
		m_pServer->Stop();
	else
		m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;
	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");
}

void Server::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Server* pServer = DBG_NEW Server();
        
        return pServer;
    }
}
