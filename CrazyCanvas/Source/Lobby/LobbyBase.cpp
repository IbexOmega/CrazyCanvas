#include "Lobby/LobbyBase.h"

#include "Application/API/Events/EventQueue.h"

using namespace LambdaEngine;

LobbyBase::LobbyBase()
{
	/*EventQueue::RegisterEventHandler<PlayerJoinedEvent>(this, &LobbyBase::OnPlayerJoinedEvent);
	EventQueue::RegisterEventHandler<PlayerLeftEvent>(this, &LobbyBase::OnPlayerLeftEvent);*/
}

LobbyBase::~LobbyBase()
{
	/*EventQueue::UnregisterEventHandler<PlayerJoinedEvent>(this, &LobbyBase::OnPlayerJoinedEvent);
	EventQueue::UnregisterEventHandler<PlayerLeftEvent>(this, &LobbyBase::OnPlayerLeftEvent);*/
}