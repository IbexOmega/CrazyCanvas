#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/ClientTCP.h"

#include "ClientTCPHandler.h"

Server::Server()
{
	using namespace LambdaEngine;
    
	m_pServerTCP = DBG_NEW ServerTCP(2, this);
	m_pServerTCP->Start(PlatformSocketFactory::GetLocalAddress(), 4444);

	/*m_pServerUDP = new ServerUDP(this);
	m_pServerUDP->Start(PlatformSocketFactory::GetLocalAddress(), 4444);*/

	UpdateTitle();
}

Server::~Server()
{
	m_pServerTCP->Release();
	//delete m_pServerUDP;

	for (LambdaEngine::IClientTCPHandler* handler : m_ClientHandlers)
		delete handler;
}

void Server::OnPacketReceivedUDP(LambdaEngine::NetworkPacket* packet, const std::string& address, uint16 port)
{
	LOG_MESSAGE("UDP Packet Received from: %s:%d", address.c_str(), port);
}

LambdaEngine::IClientTCPHandler* Server::CreateClientHandler()
{
	ClientTCPHandler* handler = DBG_NEW ClientTCPHandler();
	m_ClientHandlers.insert(handler);
	return handler;
}

bool Server::OnClientAccepted(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientAccepted");
	return true;
}

void Server::OnClientConnected(LambdaEngine::ClientTCP* client)
{
	using namespace LambdaEngine;
	LOG_MESSAGE("OnClientConnected");
	UpdateTitle();
}

void Server::OnClientDisconnected(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnected");
	UpdateTitle();
}

void Server::OnKeyDown(LambdaEngine::EKey key)
{
	m_pServerTCP->Stop();
	//m_pServerUDP->Stop();
	LOG_MESSAGE("Key Pressed: %d", key);
}

void Server::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Server::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;

	std::string title = "Server - " + std::to_string(m_pServerTCP->GetNrOfClients());

	PlatformApplication::Get()->GetWindow()->SetTitle(title.c_str());
	PlatformConsole::SetTitle(title.c_str());
}

void Server::Tick(LambdaEngine::Timestamp dt)
{

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
