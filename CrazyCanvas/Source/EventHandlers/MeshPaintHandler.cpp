#include "EventHandlers/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Rendering/PaintMaskRenderer.h"

MeshPaintHandler::~MeshPaintHandler()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ProjectileHitEvent, MeshPaintHandler>(this, &MeshPaintHandler::OnProjectileHit);
}

void MeshPaintHandler::Init()
{
	using namespace LambdaEngine;
	EventQueue::RegisterEventHandler<ProjectileHitEvent, MeshPaintHandler>(this, &MeshPaintHandler::OnProjectileHit);
}

bool MeshPaintHandler::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;
	const CollisionInfo& collisionInfo = projectileHitEvent.CollisionInfo;
	PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, EPaintMode::PAINT);
	return true;
}
