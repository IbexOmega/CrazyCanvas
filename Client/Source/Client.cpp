#include "Client.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Networking/API/PlatformNetworkUtils.h"
#include "Networking/API/IPEndPoint.h"
#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/ISocketUDP.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Client::Client() : 
    m_Dispatcher(512, this)
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

    m_pSocketUDP = PlatformNetworkUtils::CreateSocketUDP();
}

Client::~Client()
{
    delete m_pSocketUDP;
}

void Client::OnPacketReceived(LambdaEngine::NetworkPacket* packet)
{
    using namespace LambdaEngine;

    BinaryDecoder decoder(packet);
    LOG_MESSAGE(decoder.ReadString().c_str());
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(key);

    NetworkPacket* packet = m_Dispatcher.GetFreePacket();

    BinaryEncoder encoder(packet);
    encoder.WriteString("Test Message");

    m_Dispatcher.EnqueuePacket(packet);
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

    m_Dispatcher.Dispatch(m_pSocketUDP, IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
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
