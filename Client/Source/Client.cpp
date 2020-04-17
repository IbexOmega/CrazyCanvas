#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Client::Client() : 
    m_pClient(nullptr)
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

    m_pClient = ClientUDP::Create(512);

    if (!m_pClient->Connect(IPEndPoint(IPAddress::Get("192.168.0.104"), 4444)))
    {
        LOG_ERROR("Failed to connect!");
    }
}

Client::~Client()
{
    m_pClient->Release();
}

void Client::OnPacketDelivered(LambdaEngine::NetworkPacket* packet)
{
    using namespace LambdaEngine;
    LOG_INFO("Packet delivered!");
}

void Client::OnPacketResent(LambdaEngine::NetworkPacket* packet)
{

}

void Client::OnPacketReceived(LambdaEngine::NetworkPacket* packet, const LambdaEngine::IPEndPoint& sender)
{
    using namespace LambdaEngine;

    BinaryDecoder decoder(packet);
    LOG_MESSAGE(decoder.ReadString().c_str());
}

void Client::OnKeyDown(LambdaEngine::EKey key)
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
        NetworkPacket* packet = m_pClient->GetFreePacket();
        BinaryEncoder encoder(packet);
        encoder.WriteString("Test Message");
        m_pClient->SendReliable(packet, this);
    }
}

void Client::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Client::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
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
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
