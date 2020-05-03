#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/IWindow.h"

#include "Threading/API/Thread.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

#include "ClientUDPHandler.h"

#include "Math/Random.h"

Server::Server()
{
	using namespace LambdaEngine;

	m_pServer = ServerUDP::Create(this, 100, 4096, 10);
	m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));
	//m_pServer->SetSimulatePacketLoss(0.9f);

	UpdateTitle();
}

Server::~Server()
{
	m_pServer->Release();
}

void Server::OnClientConnected(LambdaEngine::IClientUDP* pClient)
{
	UNREFERENCED_VARIABLE(pClient);
}

LambdaEngine::IClientUDPRemoteHandler* Server::CreateClientUDPHandler()
{
	return DBG_NEW ClientUDPHandler();
}

void Server::KeyPressed(LambdaEngine::EKey key, uint32 modifierMask, bool isRepeat)
{
	UNREFERENCED_VARIABLE(key);

	using namespace LambdaEngine;

	if(m_pServer->IsRunning())
		m_pServer->Stop();
	else
		m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));
}

void Server::KeyReleased(LambdaEngine::EKey key)
{
	UNREFERENCED_VARIABLE(key);
}

void Server::KeyTyped(uint32 character)
{
	UNREFERENCED_VARIABLE(character);
}

void Server::UpdateTitle()
{
	using namespace LambdaEngine;
	PlatformApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");
}

void Server::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::FixedTick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
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
