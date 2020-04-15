#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Client::Client() : 
    m_Dispatcher(512)
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

    m_pSocketUDP = PlatformNetworkUtils::CreateSocketUDP();
    m_pSocketUDP->Bind(IPEndPoint(IPAddress::ANY, 0));

    Thread::Create(std::bind(&Client::Run, this), std::bind(&Client::Terminated, this));
}

Client::~Client()
{
    delete m_pSocketUDP;
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

void Client::Run()
{
    using namespace LambdaEngine;

    int32 bytesReceived = 0;
    int32 packetsReceived = 0;
    NetworkPacket* packets[32];
    IPEndPoint sender(IPAddress::NONE, 0);

    while (m_pSocketUDP->ReceiveFrom(m_pReceiveBuffer, UINT16_MAX, bytesReceived, sender))
    {
        if (m_Dispatcher.DecodePackets(m_pReceiveBuffer, bytesReceived, packets, packetsReceived))
        {
            for (int i = 0; i < packetsReceived; i++)
            {
                OnPacketReceived(packets[i], sender);
            }
            m_Dispatcher.Free(packets, packetsReceived);
        }
    }
}

void Client::Terminated()
{

}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(key);

    NetworkPacket* packet = m_Dispatcher.GetFreePacket();

    BinaryEncoder encoder(packet);
    encoder.WriteString("Test Message");

    m_Dispatcher.EnqueuePacket(packet, this);
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

    int32 bytesWritten = 0;
    int32 bytesSent = 0;
    bool done = false;
    while (!done)
    {
        done = m_Dispatcher.EncodePackets(m_pSendBuffer, bytesWritten);
        if (bytesWritten > 0)
        {
            m_pSocketUDP->SendTo(m_pSendBuffer, bytesWritten, bytesSent, IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
        }
    }
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
