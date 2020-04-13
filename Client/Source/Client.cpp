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

Client::Client()
{
	using namespace LambdaEngine;
    
    PlatformApplication::Get()->GetWindow()->SetTitle("Client");
    PlatformConsole::SetTitle("Client Console");

    ISocketUDP* socketUDP = PlatformNetworkUtils::CreateSocketUDP();
    socketUDP->Bind(IPEndPoint(IPAddress::ANY, 4444));
}

Client::~Client()
{
	
}

void Client::OnKeyDown(LambdaEngine::EKey key)
{
	using namespace LambdaEngine;
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

void Client::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Client::FixedTick(LambdaEngine::Timestamp delta)
{
    UNREFERENCED_VARIABLE(delta);
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
