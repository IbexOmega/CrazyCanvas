#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/NetworkPacket.h"


Client::Client() : 
	m_NetworkDiscovery(this, "Drift It 3D")
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

	/*m_pClientTCP = PlatformNetworkUtils::CreateClientTCP(this);
	m_pClientTCP->Connect("192.168.0.104", 4444);

	m_pClientUDP = PlatformNetworkUtils::CreateClientUDP(this);
	m_pClientUDP->Start("192.168.0.104", 4444);*/
}

Client::~Client()
{
	/*m_pClientTCP->Release();
	m_pClientUDP->Release();*/
}

void Client::OnHostFound(const std::string& address, uint16 port, LambdaEngine::NetworkPacket* packet)
{
	LOG_WARNING("Host Found %s:%d", address.c_str(), port);
}

void Client::OnClientPacketReceivedUDP(LambdaEngine::IClientUDP* client, LambdaEngine::NetworkPacket* packet)
{
	using namespace LambdaEngine;

	if (packet->ReadPacketType() == PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
	}
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
	/*NetworkPacket* packet = DBG_NEW NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet->WriteString("Hej kompis vad heter du?");
	m_pClientTCP->SendPacket(packet);

	NetworkPacket* packet2 = DBG_NEW NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
	packet2->WriteString("Hej kompis vad heter du?");
	m_pClientUDP->SendPacket(packet2);*/

	//m_pClient->Disconnect();
}

void Client::OnKeyHeldDown(LambdaEngine::EKey key)
{
	
}

void Client::OnKeyUp(LambdaEngine::EKey key)
{
	
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
