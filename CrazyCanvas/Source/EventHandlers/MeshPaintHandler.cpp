#include "EventHandlers/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"
#include "Rendering/PaintMaskRenderer.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"

/* 
* Helpers:
*/

#define SET_TEAM_INDEX(mask, value) \
	mask |= (((uint8)value) & 15) << 0

#define SET_PAINT_MODE(mask, value) \
	mask |= (((uint8)value) & 7) << 4

#define SET_WAS_SERVER(mask, value) \
	mask |= (((uint8)value) & 1) << 7

#define GET_TEAM_INDEX(mask) (ETeam)(mask & 15)
#define GET_PAINT_MODE(mask) (EPaintMode)((mask & 7) >> 4)
#define GET_WAS_SERVER(mask) (bool)((mask & 1) >> 7)

/*
* MeshPaintHandler code:
*/

MeshPaintHandler::~MeshPaintHandler()
{
	using namespace LambdaEngine;
	EventQueue::UnregisterEventHandler<ProjectileHitEvent, MeshPaintHandler>(this, &MeshPaintHandler::OnProjectileHit);
	EventQueue::UnregisterEventHandler<PacketReceivedEvent<PacketProjectileHit>>(this, &MeshPaintHandler::OnPacketProjectileHitReceived);
}

void MeshPaintHandler::Init()
{
	using namespace LambdaEngine;
	EventQueue::RegisterEventHandler<ProjectileHitEvent, MeshPaintHandler>(this, &MeshPaintHandler::OnProjectileHit);
	EventQueue::RegisterEventHandler<PacketReceivedEvent<PacketProjectileHit>>(this, &MeshPaintHandler::OnPacketProjectileHitReceived);
}

bool MeshPaintHandler::OnProjectileHit(const ProjectileHitEvent& projectileHitEvent)
{
	using namespace LambdaEngine;

	if (projectileHitEvent.AmmoType != EAmmoType::AMMO_TYPE_NONE)
	{
		EPaintMode paintMode = EPaintMode::NONE;
		ETeam team = ETeam::NONE;
		ERemoteMode remoteMode = ERemoteMode::UNDEFINED;

		if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			paintMode	= EPaintMode::PAINT;
			team		= projectileHitEvent.Team;
		}
		else if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_WATER)
		{
			paintMode	= EPaintMode::REMOVE;
		}

		const EntityCollisionInfo& collisionInfo = projectileHitEvent.CollisionInfo0;

		/*if (MultiplayerUtils::IsServer())
		{
			PacketProjectileHit packet;
			SET_TEAM_INDEX(packet.Info, team);
			SET_PAINT_MODE(packet.Info, paintMode);
			SET_WAS_SERVER(packet.Info, true);
			packet.Position		= collisionInfo.Position;
			packet.Direction	= collisionInfo.Direction;

			ServerHelper::SendBroadcast(packet);
		}
		else
		{
			PacketProjectileHit packet;
			SET_TEAM_INDEX(packet.Info, team);
			SET_PAINT_MODE(packet.Info, paintMode);
			SET_WAS_SERVER(packet.Info, false);
			packet.Position = collisionInfo.Position;
			packet.Direction = collisionInfo.Direction;

			ClientHelper::Send(packet);

		}*/
		LOG_INFO("Hit! Was server: %s", MultiplayerUtils::IsServer() ? "True" : "False");
			remoteMode = ERemoteMode::CLIENT;
			PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team);
	}

	return true;
}

bool MeshPaintHandler::OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event)
{
	using namespace LambdaEngine;

	const PacketProjectileHit& packet = event.Packet;
	ETeam		team		= GET_TEAM_INDEX(packet.Info);
	EPaintMode	paintMode	= GET_PAINT_MODE(packet.Info);
	bool		wasServer	= GET_WAS_SERVER(packet.Info);

	ERemoteMode remoteMode = ERemoteMode::UNDEFINED;
	if (wasServer)
	{
		remoteMode = ERemoteMode::SERVER;
	}
	else
	{
		// Check if it was correct, else change it.
	}

	PaintMaskRenderer::AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team);
	return true;
}
