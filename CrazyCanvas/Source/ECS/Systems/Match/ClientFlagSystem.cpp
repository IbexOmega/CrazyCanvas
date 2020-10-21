#include "ECS/Systems/Match/ClientFlagSystem.h"

ClientFlagSystem::ClientFlagSystem()
{
}

ClientFlagSystem::~ClientFlagSystem()
{
}

void ClientFlagSystem::OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity)
{
}

void ClientFlagSystem::OnFlagDropped()
{
}

void ClientFlagSystem::OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	//Handle Flag Collision
	LOG_WARNING("FLAG COLLIDED Client");
}

void ClientFlagSystem::TickInternal(LambdaEngine::Timestamp deltaTime)
{
}
