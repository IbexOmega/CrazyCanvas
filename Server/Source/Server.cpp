#include "Server.h"

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

Server::Server() :
	m_Dispatcher(512, this)
{
	using namespace LambdaEngine;

	m_pSocketUDP = PlatformNetworkUtils::CreateSocketUDP();
	m_pSocketUDP->Bind(IPEndPoint(IPAddress::ANY, 4444));

	UpdateTitle();
}

Server::~Server()
{
	delete m_pSocketUDP;
}

void Server::OnPacketReceived(LambdaEngine::NetworkPacket* packet)
{
	using namespace LambdaEngine;

	BinaryDecoder decoder(packet);
	LOG_MESSAGE(decoder.ReadString().c_str());
}

void Server::OnKeyDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyHeldDown(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::OnKeyUp(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;
	PlatformApplication::Get()->GetWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");
}

void Server::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::FixedTick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;
    UNREFERENCED_VARIABLE(delta);

	m_Dispatcher.Receive(m_pSocketUDP);
	m_Dispatcher.Dispatch(m_pSocketUDP, IPEndPoint(IPAddress::Get("192.168.0.104"), 4444));
}

namespace LambdaEngine
{
    Game* CreateGame()
    {
		Server* pSandbox = DBG_NEW Server();
        Input::AddKeyboardHandler(pSandbox);
        
        return pSandbox;
    }
}
