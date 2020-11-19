#pragma once

#include "ECS/Entity.h"

#include "Rendering/PaintMaskRenderer.h"
#include "ECS/Systems/Player/HealthSystemServer.h"

// KillPlaneCallback is called when an entity falls behind a kill plane. Entity0 is the kill plane entity.
inline void ParticleShowerCallback(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
{
	UNREFERENCED_VARIABLE(entity0);
	LOG_INFO("Entity %d is taking a shower", entity1);

	LambdaEngine::PaintMaskRenderer::ResetServer(entity1);
	//HealthSystemServer::ResetHealth(entity1);
}
