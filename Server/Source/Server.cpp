#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Threading/API/Thread.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/NetworkPacket.h"
#include "Networking/API/BinaryEncoder.h"
#include "Networking/API/BinaryDecoder.h"

Server::Server()
{
	using namespace LambdaEngine;

	m_pServer = ServerUDP::Create(512);
	m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));

	UpdateTitle();
}

Server::~Server()
{
	m_pServer->Release();
}

void Server::OnPacketReceived(LambdaEngine::NetworkPacket* packet, const LambdaEngine::IPEndPoint& sender)
{
	using namespace LambdaEngine;

	BinaryDecoder decoder(packet);
	LOG_MESSAGE(decoder.ReadString().c_str());

	/*NetworkPacket* response = m_pServer->GetFreePacket();
	BinaryEncoder encoder(response);
	encoder.WriteString("I got your message");
	m_pServer->SendUnreliable(response);*/
}

void Server::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
	UNREFERENCED_VARIABLE(key);

	if(m_pServer->IsRunning())
		m_pServer->Stop();
	else
		m_pServer->Start(IPEndPoint(IPAddress::ANY, 4444));
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

	m_pServer->Flush();
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