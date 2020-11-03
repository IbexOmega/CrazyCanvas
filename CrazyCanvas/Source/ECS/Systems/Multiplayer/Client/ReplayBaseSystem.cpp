#include "ECS/Systems/Multiplayer/Client/ReplayBaseSystem.h"
#include "ECS/Systems/Multiplayer/Client/ReplayManagerSystem.h"

#include "Game/Multiplayer/MultiplayerUtils.h"

using namespace LambdaEngine;

void ReplayBaseSystem::RegisterSystem(const String& systemName, SystemRegistration& systemRegistration)
{
	System::RegisterSystem(systemName, systemRegistration);

	ASSERT(!MultiplayerUtils::IsServer());
	ReplayManagerSystem::GetInstance()->RegisterReplaySystem(this);
}