#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/IWindow.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

#include "Networking/API/PacketTransceiver.h"


Client::Client() : 
    m_pClient(nullptr)
{
	using namespace LambdaEngine;

	PlatformApplication::Get()->AddEventHandler(this);
    
    PlatformApplication::Get()->GetMainWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");


    m_pClient = ClientUDP::Create(this, 1024, 100);

    if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
    {
        LOG_ERROR("Failed to connect!");
    }
}

Client::~Client()
{
    m_pClient->Release();
}

void Client::OnConnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnConnectingUDP()");
}

void Client::OnConnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    using namespace LambdaEngine;

    LOG_MESSAGE("OnConnectedUDP()");

    for (int i = 0; i < 1; i++)
    {
        NetworkPacket* pPacket = m_pClient->GetFreePacket(1);
        BinaryEncoder encoder(pPacket);
        encoder.WriteInt32(i);
        m_pClient->SendReliable(pPacket, this);
    }
}

void Client::OnDisconnectingUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnectingUDP()");
}

void Client::OnDisconnectedUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_MESSAGE("OnDisconnectedUDP()");
}

void Client::OnPacketReceivedUDP(LambdaEngine::IClientUDP* pClient, LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_VARIABLE(pClient);
    UNREFERENCED_VARIABLE(pPacket);
    LOG_MESSAGE("OnPacketReceivedUDP()");
}

void Client::OnServerFullUDP(LambdaEngine::IClientUDP* pClient)
{
    UNREFERENCED_VARIABLE(pClient);
    LOG_ERROR("OnServerFullUDP()");
}

void Client::OnPacketDelivered(LambdaEngine::NetworkPacket* pPacket)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketDelivered()");
}

void Client::OnPacketResent(LambdaEngine::NetworkPacket* pPacket, uint8 tries)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_INFO("OnPacketResent(%d)", tries);
}

void Client::OnPacketMaxTriesReached(LambdaEngine::NetworkPacket* pPacket, uint8 tries)
{
    UNREFERENCED_VARIABLE(pPacket);
    LOG_ERROR("OnPacketMaxTriesReached(%d)", tries);
}

void Client::KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(key);

    if (key == EKey::KEY_ENTER)
    {
        if (m_pClient->IsConnected())
            m_pClient->Disconnect();
        else
            m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
    }
    else
    {
        uint16 packetType = 0;
        NetworkPacket* packet = m_pClient->GetFreePacket(packetType);
        BinaryEncoder encoder(packet);
        encoder.WriteString("Test Message");
        m_pClient->SendReliable(packet, this);
    }
}

void Client::KeyReleased(LambdaEngine::EKey key)
{
    UNREFERENCED_VARIABLE(key);
}

void Client::KeyTyped(uint32 character) 
{
    UNREFERENCED_VARIABLE(character);
}

void Client::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Client::FixedTick(LambdaEngine::Timestamp delta)
{
    using namespace LambdaEngine;
    UNREFERENCED_VARIABLE(delta);

    m_pClient->Flush();
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Client* pSandbox = DBG_NEW Client();
        
        return pSandbox;
    }
}
