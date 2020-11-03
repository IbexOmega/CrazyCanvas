#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"

#include "ECS/Systems/Multiplayer/Client/ReplaySystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

using namespace LambdaEngine;

void ReplayBaseSystem::RegisterSystem(const String& systemName, SystemRegistration& systemRegistration)
{
	System::RegisterSystem(systemName, systemRegistration);

	ASSERT(!MultiplayerUtils::IsServer());
	ReplaySystem::GetInstance()->RegisterReplaySystem(this);
}
