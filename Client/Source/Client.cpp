#include "Client.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"

Client::Client()
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");
    
    m_pClient = new ClientTCP(this);
	m_pClient->Connect("192.168.0.100", 4444);
    
//    ISocketTCP* pSocket = PlatformSocketFactory::CreateSocketTCP();
//    if (pSocket->Connect("127.0.0.1", 4444))
//    {
//        LOG_MESSAGE("CONNECTED");
//        char buffer[512];
//        int bytesRead = 0;
//        if (pSocket->Receive(buffer, 512, bytesRead))
//        {
//            LOG_MESSAGE("Receive");
//        }
//        else
//            LOG_MESSAGE("Receive Failed");
//    }
//    else
//    {
//        LOG_MESSAGE("Failed to connect");
//    }
}

Client::~Client()
{
}

void Client::OnClientConnected(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnected");
}

void Client::OnClientDisconnected(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnected");
}

void Client::OnClientFailedConnecting(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientFailedConnecting");
}

void Client::OnClientPacketReceived(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	m_pClient->Disconnect();
	LOG_MESSAGE("Key Pressed: %d", key);
}

void Client::OnKeyHeldDown(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Held Down: %d", key);
}

void Client::OnKeyUp(LambdaEngine::EKey key)
{
	LOG_MESSAGE("Key Released: %d", key);
}

void Client::Tick()
{
	using namespace LambdaEngine;
	NetworkPacket* packet = new NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet->WriteString("Hej kompis vad heter du?");

	m_pClient->SendPacket(packet);
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Client* pSandbox = new Client();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
