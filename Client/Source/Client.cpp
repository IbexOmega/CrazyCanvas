#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Network/API/NetworkPacket.h"

enum ENetworkTest
{
	NETWORK_TEST_TCP,
	NETWORK_TEST_UDP,
	NETWORK_TEST_DISCOVERY
};

ENetworkTest g_Test = NETWORK_TEST_TCP;

Client::Client()
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

	if (g_Test == NETWORK_TEST_TCP)
	{
		m_pClientTCP = PlatformNetworkUtils::CreateClientTCP(this);
		m_pClientTCP->Connect("192.168.0.104", 4444);
	}
	else if (g_Test == NETWORK_TEST_UDP)
	{
		m_pClientUDP = PlatformNetworkUtils::CreateClientUDP(this);
		m_pClientUDP->Start("192.168.0.104", 4444);
	}
	else if (g_Test == NETWORK_TEST_DISCOVERY)
	{
		m_pNetworkDiscovery = new NetworkDiscoverySearcher(this, "Drift It 3D");
	}
}

Client::~Client()
{
	if (g_Test == NETWORK_TEST_TCP)
	{
		m_pClientTCP->Release();
	}
	else if (g_Test == NETWORK_TEST_UDP)
	{
		m_pClientUDP->Release();
	}
	else if (g_Test == NETWORK_TEST_DISCOVERY)
	{
		delete m_pNetworkDiscovery;
	}
}

void Client::OnHostFound(const std::string& address, uint16 port, LambdaEngine::NetworkPacket* packet)
{
	LOG_WARNING("Host Found %s:%d", address.c_str(), port);
	UNREFERENCED_VARIABLE(packet);
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
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientErrorUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientErrorUDP()");
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientStoppedUDP(LambdaEngine::IClientUDP* client)
{
	LOG_MESSAGE("OnClientStoppedUDP()");
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientConnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientConnectedTCP()");
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientDisconnectedTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientDisconnectedTCP()");
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientFailedConnectingTCP(LambdaEngine::ClientTCP* client)
{
	LOG_MESSAGE("OnClientFailedConnectingTCP()");
	UNREFERENCED_VARIABLE(client);
}

void Client::OnClientPacketReceivedTCP(LambdaEngine::ClientTCP* client, LambdaEngine::NetworkPacket* packet)
{
	using namespace LambdaEngine;

	PACKET_TYPE packetType = packet->ReadPacketType();
	if (packetType == PACKET_TYPE_SERVER_FULL)
	{
		LOG_WARNING("Server Full");
	}
	else if (packetType == PACKET_TYPE_USER_DATA)
	{
		std::string str;
		packet->ReadString(str);
		LOG_MESSAGE(str.c_str());
	}
	UNREFERENCED_VARIABLE(client);
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;

	if (g_Test == NETWORK_TEST_TCP)
	{
		NetworkPacket* packet = DBG_NEW NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
		packet->WriteString("Test Messsage TCP");
		m_pClientTCP->SendPacket(packet, true);
	}
	else if (g_Test == NETWORK_TEST_UDP)
	{
		NetworkPacket* packet = DBG_NEW NetworkPacket(EPacketType::PACKET_TYPE_USER_DATA);
		packet->WriteString("Test Messsage UDP");
		m_pClientUDP->SendPacket(packet, true);
	}
	UNREFERENCED_VARIABLE(key);
}

void Client::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Client::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Client::Tick(LambdaEngine::Timestamp dt)
{
	UNREFERENCED_VARIABLE(dt);
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
