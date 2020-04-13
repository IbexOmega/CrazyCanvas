#include "Server.h"

#include "Memory/Memory.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/PlatformApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"

#include "Networking/API/ISocket.h"
#include "Networking/API/IPEndPoint.h"

Server::Server()
{
	using namespace LambdaEngine;

	UpdateTitle();
}

Server::~Server()
{
	
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
