#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/PlatformSocketFactory.h"
#include "Network/API/NetworkPacket.h"

#include "Network/API/ClientUDP.h"

Client::Client()
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

	m_pClientTCP = new ClientTCP(this);
	m_pClientTCP->Connect("192.168.0.104", 4444);

	m_pClientUDP = new ClientUDP("192.168.0.104", 4444, this);
}

Client::~Client()
{
	m_pClientTCP->Release();
	m_pClientUDP->Release();
}

void Client::OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet)
{
	LOG_MESSAGE("UDP Packet Received");
}

void Client::OnClientErrorUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientErrorUDP");
}

void Client::OnClientStoppedUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientStoppedUDP");
}

void Client::OnClientConnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnected");
}

void Client::OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnected");
}

void Client::OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientFailedConnecting");
}

void Client::OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
	using namespace LambdaEngine;

	PACKET_TYPE packetType = packet->ReadPacketType();
	if (packetType == PACKET_TYPE_SERVER_FULL)
	{
		LOG_WARNING("Server Full");
	}
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	NetworkPacket* packet = DBG_NEW NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet->WriteString("Hej kompis vad heter du?");
	m_pClientTCP->SendPacket(packet);

	NetworkPacket* packet2 = new NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet2->WriteString("Hej kompis vad heter du?");
	m_pClientUDP->SendPacket(packet2);

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

void Client::Tick(LambdaEngine::Timestamp dt)
{
	
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Client* pSandbox = DBG_NEW Client();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
