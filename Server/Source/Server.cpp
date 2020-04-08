#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/PlatformNetworkUtils.h"

#include "ClientTCPHandler.h"
#include "ClientUDPHandler.h"

enum ENetworkTest
{
	NETWORK_TEST_TCP,
	NETWORK_TEST_UDP,
	NETWORK_TEST_DISCOVERY
};

ENetworkTest g_Test = NETWORK_TEST_TCP;

Server::Server()
{
	using namespace LambdaEngine;

	if (g_Test == NETWORK_TEST_TCP)
	{
		m_pServerTCP = PlatformNetworkUtils::CreateServerTCP(this, 2);
		m_pServerTCP->Start(PlatformNetworkUtils::GetLocalAddress(), 4444);
	}
	else if (g_Test == NETWORK_TEST_UDP)
	{
		m_pServerUDP = PlatformNetworkUtils::CreateServerUDP(this);
		m_pServerUDP->Start(PlatformNetworkUtils::GetLocalAddress(), 4444);
	}
	else if (g_Test == NETWORK_TEST_DISCOVERY)
	{
		m_pNetworkDiscovery = new NetworkDiscoveryHost(this, "Drift It 3D", LambdaEngine::PlatformNetworkUtils::GetLocalAddress(), 4444);
	}

	UpdateTitle();
}

Server::~Server()
{
	if (g_Test == NETWORK_TEST_TCP)
	{
		m_pServerTCP->Release();
	}
	else if (g_Test == NETWORK_TEST_UDP)
	{
		m_pServerUDP->Release();
	}
	else if (g_Test == NETWORK_TEST_DISCOVERY)
	{
		delete m_pNetworkDiscovery;
	}

	for (LambdaEngine::IClientTCPHandler* handler : m_ClientTCPHandlers)
		delete handler;

	for (LambdaEngine::IClientUDPHandler* handler : m_ClientUDPHandlers)
		delete handler;
}

void Server::OnSearcherRequest(LambdaEngine::NetworkPacket* packet)
{
	UNREFERENCED_VARIABLE(packet);
}

LambdaEngine::IClientUDPHandler* Server::CreateClientHandlerUDP()
{
	ClientUDPHandler* handler = DBG_NEW ClientUDPHandler();
	m_ClientUDPHandlers.insert(handler);
	return handler;
}

LambdaEngine::IClientTCPHandler* Server::CreateClientHandlerTCP()
{
	ClientTCPHandler* handler = DBG_NEW ClientTCPHandler();
	m_ClientTCPHandlers.insert(handler);
	return handler;
}

bool Server::OnClientAcceptedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientAcceptedTCP()");
	UNREFERENCED_VARIABLE(client);
	return true;
}

void Server::OnClientConnectedTCP(LambdaEngine::ClientTCP* client)
{
	UNREFERENCED_VARIABLE(client);
	UpdateTitle();
}

void Server::OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client)
{
	UNREFERENCED_VARIABLE(client);
	UpdateTitle();
}

void Server::OnKeyDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;

	if (g_Test == NETWORK_TEST_TCP)
	{
		std::string title = "Server - " + std::to_string(m_pServerTCP->GetNrOfClients());

		PlatformApplication::Get()->GetWindow()->SetTitle(title.c_str());
		PlatformConsole::SetTitle(title.c_str());
	}
}

void Server::Tick(LambdaEngine::Timestamp dt)
{
	UNREFERENCED_VARIABLE(dt);
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Server* pSandbox = DBG_NEW Server();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
