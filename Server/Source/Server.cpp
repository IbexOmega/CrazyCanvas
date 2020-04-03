#include "Server.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/ClientTCP.h"

Server::Server()
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Server");
    PlatformConsole::SetTitle("Server Console");
    
//    ISocketTCP* server = PlatformSocketFactory::CreateSocketTCP();
//    if(server->Bind("127.0.0.1", 4444))
//    {
//        LOG_MESSAGE("BIND");
//    }
//    if(server->Listen())
//    {
//        LOG_MESSAGE("LISTEN");
//    }
//    ISocketTCP* socket = server->Accept();
//    if(socket)
//    {
//        LOG_MESSAGE("ACCEPT");
//        char buffer[512];
//        int bytesRead = 0;
//        while(true)
//        {
//            if(socket->Receive(buffer, 512, bytesRead))
//            {
//                LOG_MESSAGE("RECEIVED %d BYTES", bytesRead);
//            }
//        }
//    }
	m_pServer = new ServerTCP(this);
	m_pServer->Start("192.168.0.100", 4444);
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
