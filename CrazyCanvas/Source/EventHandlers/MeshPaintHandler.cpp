#include "EventHandlers/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"

/*
* MeshPaintHandler
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
			paintMode = EPaintMode::PAINT;
			team = projectileHitEvent.Team;
		}
		else if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_WATER)
		{
			paintMode = EPaintMode::REMOVE;
		}

		const EntityCollisionInfo& collisionInfo = projectileHitEvent.CollisionInfo0;
		if (MultiplayerUtils::IsServer())
		{
			remoteMode = ERemoteMode::SERVER;
			PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team, projectileHitEvent.Angle);

			// Send the server's paint point to all clients.
			PacketProjectileHit packet;
			SET_TEAM_INDEX(packet.Info, team);
			SET_PAINT_MODE(packet.Info, paintMode);
			packet.Position		= collisionInfo.Position;
			packet.Direction	= collisionInfo.Direction;
			packet.Angle		= projectileHitEvent.Angle;
			ServerHelper::SendBroadcast(packet);
		}
		else
		{
			// If it is a client, paint it on the temporary mask and save the point.
			remoteMode = ERemoteMode::CLIENT;
			PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team, projectileHitEvent.Angle);
		}
	}

	return true;
}

bool MeshPaintHandler::OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event)
{
	using namespace LambdaEngine;

	const PacketProjectileHit& packet = event.Packet;
	ETeam		team		= GET_TEAM_INDEX(packet.Info);
	EPaintMode	paintMode	= GET_PAINT_MODE(packet.Info);
	ERemoteMode remoteMode	= ERemoteMode::UNDEFINED;

	if (!MultiplayerUtils::IsServer())
	{
		// Allways clear client side when receiving hit from the server.
		PaintMaskRenderer::ResetClient();

		// Allways paint the server's paint point to the server side mask (permanent mask)
		remoteMode = ERemoteMode::SERVER;
		PaintMaskRenderer::AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team, packet.Angle);
	}

	return true;
}
