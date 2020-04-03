#include "Server.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/ClientTCP.h"

Server::Server()
{
	using namespace LambdaEngine;
    
	m_pServer = new ServerTCP(this);
	m_pServer->Start("192.168.0.104", 4444);
}

Server::~Server()
{
}

bool Server::OnClientAccepted(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientAccepted");
	return true;
}

void Server::OnClientConnected(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnected");
}

void Server::OnClientDisconnected(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnected");
}



void Server::OnKeyDown(LambdaEngine::EKey key)
{
	m_pServer->Stop();
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

void Server::Tick()
{
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Server* pSandbox = new Server();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
