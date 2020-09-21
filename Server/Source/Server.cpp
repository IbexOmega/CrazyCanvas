#include "Server.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"
#include "Application/API/Events/EventQueue.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/NetworkDebugger.h"

#include "ClientHandler.h"

#include "Math/Random.h"

#include <argh/argh.h>

using namespace LambdaEngine;

Server::Server()
{
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &Server::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	ServerDesc desc = {};
	desc.Handler		= this;
	desc.MaxRetries		= 10;
	desc.MaxClients		= 10;
	desc.PoolSize		= 32000;
	desc.Protocol		= EProtocol::UDP;
	desc.PingInterval	= Timestamp::Seconds(1);
	desc.PingTimeout	= Timestamp::Seconds(3);
	desc.UsePingSystem	= false;

	m_pServer = NetworkUtils::CreateServer(desc);
	m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));

	//((ServerUDP*)m_pServer)->SetSimulateReceivingPacketLoss(0.1f);
}

Server::~Server()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &Server::OnKeyPressed);
	m_pServer->Release();
}

LambdaEngine::IClientRemoteHandler* Server::CreateClientHandler()
{
	return DBG_NEW ClientHandler();
}

bool Server::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);

	if(m_pServer->IsRunning())
		m_pServer->Stop("User Requested");
	else
		m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));

	return false;
}

void Server::UpdateTitle()
{
	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");
}

void Server::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	NetworkDebugger::RenderStatistics(m_pServer);
}

void Server::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	// Simulate first Remote client broadcasting
	if (m_pServer->GetClientCount() > 0)
	{
		ClientRemoteBase* pClient = m_pServer->GetClients().begin()->second;
		if (pClient->IsConnected())
		{
			NetworkSegment* pPacket = pClient->GetFreePacket(99);
			BinaryEncoder encoder(pPacket);
			encoder.WriteString("Test broadcast from server.cpp");
			pClient->SendUnreliableBroadcast(pPacket);
		}
	}
}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		UNREFERENCED_VARIABLE(flagParser);
		Server* pServer = DBG_NEW Server();
		return pServer;
	}
}
