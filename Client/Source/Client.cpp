#include "Client.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"

Client::Client()
{
	using namespace LambdaEngine;
    
	m_pClient = new ClientTCP(this);
	m_pClient->Connect("192.168.0.104", 4444);
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
	using namespace LambdaEngine;
	NetworkPacket* packet = new NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet->WriteString("Hej kompis vad heter du?");

	m_pClient->SendPacket(packet);

	//m_pClient->Disconnect();
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
