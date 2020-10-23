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

	if (projectileHitEvent.AmmoType != EAmmoType::AMMO_TYPE_NONE)
	{
		EPaintMode paintMode;
		ETeam team;
		ERemoteMode remoteMode;

		if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			paintMode = EPaintMode::PAINT;
			team = ETeam::RED;
			remoteMode = ERemoteMode::SERVER;
		}
		else if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_WATER)
		{
			paintMode = EPaintMode::PAINT;
			team = ETeam::BLUE;
			remoteMode = ERemoteMode::CLIENT;
		}

		const EntityCollisionInfo& collisionInfo = projectileHitEvent.CollisionInfo0;
		PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team);
	}

	return true;
}
