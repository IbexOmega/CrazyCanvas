#include "Server.h"

#include "Memory/API/Malloc.h"

#include "Log/Log.h"

#include "Input/API/Input.h"

#include "Application/API/PlatformMisc.h"
#include "Application/API/CommonApplication.h"
#include "Application/API/PlatformConsole.h"
#include "Application/API/Window.h"
#include "Application/API/Events/EventQueue.h"

#include "Threading/API/Thread.h"

#include "Networking/API/PlatformNetworkUtils.h"

#include "Math/Random.h"

#include <argh/argh.h>

#include "Game/ECS/Systems/Networking/ServerSystem.h"

using namespace LambdaEngine;

Server::Server()
{
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &Server::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");


	//NetworkDiscovery::EnableServer("Crazy Canvas", 4444, this);

	ServerSystem::GetInstance().Start();
}

Server::~Server()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &Server::OnKeyPressed);
}

bool Server::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

void Server::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::FixedTick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void Server::OnNetworkDiscoveryPreTransmit(const BinaryEncoder& encoder)
{
	UNREFERENCED_VARIABLE(encoder);
}

namespace LambdaEngine
{
	Game* CreateGame(const argh::parser& flagParser)
	{
		UNREFERENCED_VARIABLE(flagParser);
		Server* pServer = DBG_NEW Server();
		return pServer;
	}
}
