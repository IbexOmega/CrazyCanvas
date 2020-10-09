#include "States/ServerState.h"

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

ServerState::~ServerState()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);
}

void ServerState::Init()
{
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &ServerState::OnKeyPressed);

	CommonApplication::Get()->GetMainWindow()->SetTitle("Server");
	PlatformConsole::SetTitle("Server Console");

	//NetworkDiscovery::EnableServer("Crazy Canvas", 4444, this);

	ServerSystem::GetInstance().Start();
}

bool ServerState::OnKeyPressed(const KeyPressedEvent& event)
{
	UNREFERENCED_VARIABLE(event);
	return false;
}

void ServerState::Tick(Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}

void ServerState::OnNetworkDiscoveryPreTransmit(const BinaryEncoder& encoder)
{
	UNREFERENCED_VARIABLE(encoder);
}
