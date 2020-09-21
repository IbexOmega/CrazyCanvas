#include "Client.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Resources/ResourceManager.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioSystem.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/NetworkDebugger.h"

#include <argh/argh.h>

using namespace LambdaEngine;

Client::Client() :
	m_pClient(nullptr),
	m_IsBenchmarking(false),
	m_BenchmarkPackets(0)
{

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Client");
	PlatformConsole::SetTitle("Client Console");

    /*ClientDesc desc = {};
    desc.PoolSize               = 2048;
    desc.MaxRetries             = 10;
    desc.ResendRTTMultiplier    = 2.0F;
    desc.Handler                = this;
    desc.Protocol               = EProtocol::UDP;
	desc.PingInterval			= Timestamp::Seconds(1);
	desc.PingTimeout			= Timestamp::Seconds(3);
	desc.UsePingSystem			= true;

	m_pClient = NetworkUtils::CreateClient(desc);

	if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
	{
		LOG_ERROR("Failed to connect!");
	}*/

	NetworkDiscovery::EnableClient("Crazy Canvas", this);
}

Client::~Client()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &Client::OnKeyPressed);

	if(m_pClient)
		m_pClient->Release();
}

void Client::OnServerFound(LambdaEngine::BinaryDecoder decoder, uint16 portOfGameServer)
{
	LOG_MESSAGE("OnServerFound(%d)", portOfGameServer);
}

void Client::OnConnecting(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnecting()");
}

void Client::OnConnected(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnConnected()");
}

void Client::OnDisconnecting(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnecting()");
}

void Client::OnDisconnected(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_MESSAGE("OnDisconnected()");
}

void Client::OnPacketReceived(IClient* pClient, NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pClient);
	UNREFERENCED_VARIABLE(pPacket);
	LOG_MESSAGE("OnPacketReceived(%s)", pPacket->ToString().c_str());
}

void Client::OnServerFull(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_ERROR("OnServerFull()");
}

void Client::OnClientReleased(IClient* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
	LOG_ERROR("OnClientReleased()");
}

void Client::OnPacketDelivered(NetworkSegment* pPacket)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_INFO("OnPacketDelivered(%s)", pPacket->ToString().c_str());
}

void Client::OnPacketResent(NetworkSegment* pPacket, uint8 tries)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_INFO("OnPacketResent(%d)", tries);
}

void Client::OnPacketMaxTriesReached(NetworkSegment* pPacket, uint8 tries)
{
	UNREFERENCED_VARIABLE(pPacket);
	LOG_ERROR("OnPacketMaxTriesReached(%d)", tries);
}

bool Client::OnKeyPressed(const KeyPressedEvent& event)
{
	if (event.Key == EKey::KEY_ENTER)
	{
		if (m_pClient->IsConnected())
			m_pClient->Disconnect("User Requested");
		else
			m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.1.65"), 4444));
	}
	else if(event.Key == EKey::KEY_HOME)
	{
		m_IsBenchmarking = true;
	}

	return false;
}

void Client::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

	if (m_pClient)
	{
		if (m_pClient->IsConnected() && m_IsBenchmarking)
		{
			RunningBenchMark();
		}
		NetworkDebugger::RenderStatistics(m_pClient);
	}
}

void Client::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);

}

void Client::RunningBenchMark()
{
	if (m_BenchmarkPackets++ < 100000)
	{
		NetworkSegment* pPacket = m_pClient->GetFreePacket(420);
		BinaryEncoder encoder(pPacket);
		encoder.WriteUInt32(m_BenchmarkPackets);
		m_pClient->SendReliable(pPacket, this);
	}
	else
	{
		m_IsBenchmarking = false;
		m_BenchmarkPackets = 0;
	}
}


namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& parser)
	{
		UNREFERENCED_VARIABLE(parser);
		Client* pClient = DBG_NEW Client();
		return pClient;
	}
}
