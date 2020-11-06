#include "EventHandlers/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"

/*
* Helpers
*/

#define SET_TEAM_INDEX(mask, value) \
	mask |= (((uint8)value) & 15) << 0

#define SET_PAINT_MODE(mask, value) \
	mask |= (((uint8)value) & 7) << 4

#define SET_WAS_SERVER(mask, value) \
	mask |= (((uint8)value) & 1) << 7

#define GET_TEAM_INDEX(mask) (ETeam)(mask & 15)
#define GET_PAINT_MODE(mask) (EPaintMode)((mask >> 4) & 7)
#define GET_WAS_SERVER(mask) (bool)((mask >> 7) & 1)

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
			// Send the server's paint point to all clients.
			PacketProjectileHit packet;
			SET_TEAM_INDEX(packet.Info, team);
			LOG_WARNING("[SERVER] Paint with paint mode %s", paintMode == EPaintMode::REMOVE ? "REMOVE" : (paintMode == EPaintMode::PAINT ? "PAINT" : "NONE"));
			SET_PAINT_MODE(packet.Info, paintMode);
			SET_WAS_SERVER(packet.Info, true);
			packet.Position		= collisionInfo.Position;
			packet.Direction	= collisionInfo.Direction;

			ServerHelper::SendBroadcast(packet);
		}
		else
		{
			// If it is a client, paint it on the temporary mask and save the point.
			remoteMode = ERemoteMode::CLIENT;
			PaintMaskRenderer::AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team);

			PaintPoint paintPoint = {};
			paintPoint.Position		= collisionInfo.Position;
			paintPoint.Direction	= collisionInfo.Direction;
			paintPoint.PaintMode	= paintMode;
			paintPoint.RemoteMode	= remoteMode;
			paintPoint.Team			= team;
			m_PaintPointsOnClient.push(paintPoint);
		}
		LOG_INFO("paintMode: %d, remoteMode: %d, team: %d", (uint32)paintMode, (uint32)remoteMode, (uint32)team);

	}

	return true;
}

bool MeshPaintHandler::OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event)
{
	using namespace LambdaEngine;

	const PacketProjectileHit& packet = event.Packet;
	ETeam		team = GET_TEAM_INDEX(packet.Info);
	EPaintMode	paintMode = GET_PAINT_MODE(packet.Info);
	bool		wasServer = GET_WAS_SERVER(packet.Info);
	ERemoteMode remoteMode = ERemoteMode::UNDEFINED;

	if (!MultiplayerUtils::IsServer())
	{
		bool clientWasWrong = false;
		if (m_PaintPointsOnClient.empty())
		{
			clientWasWrong = true;
			D_LOG_ERROR("Prediction Error: Client did not hit but server did, paint server's data to mask...");
		}
		else
		{
			PaintPoint paintPointA = m_PaintPointsOnClient.front();
			m_PaintPointsOnClient.pop();

			PaintPoint paintPointB = {};
			paintPointB.Position = packet.Position;
			paintPointB.Direction = packet.Direction;
			paintPointB.PaintMode = paintMode;
			paintPointB.RemoteMode = remoteMode;
			paintPointB.Team = team;
			clientWasWrong = IsPaintPointEqual(paintPointA, paintPointB);
			if(clientWasWrong)
				D_LOG_ERROR("Prediction Error: Client got wrong prediction when painting, reset client side paint and repaint on server side...");
		}

		// Clear client side if all paint points have been processed.
		if (m_PaintPointsOnClient.empty())
		{
			LOG_WARNING("Clear client side mask...");
			PaintMaskRenderer::ResetClient();
		}

		// Allways paint the server's paint point to the server side mask (permanent mask)
		remoteMode = ERemoteMode::SERVER;
		LOG_WARNING("Paint with paint mode %s", paintMode == EPaintMode::REMOVE ? "REMOVE" : (paintMode == EPaintMode::PAINT ? "PAINT" : "NONE"));
		PaintMaskRenderer::AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team);
	}

	return true;
}

bool MeshPaintHandler::IsPaintPointEqual(PaintPoint& a, PaintPoint& b)
{
	bool posSame = glm::length2(a.Position - b.Position) < 0.0001f;
	bool dirSame = glm::length2(a.Direction - b.Direction) < 0.0001f;
	bool paintModeSame	= a.PaintMode == b.PaintMode;
	bool remoteModeSame = a.RemoteMode == b.RemoteMode;
	bool teamSame		= a.Team == b.Team;
	return posSame && dirSame && paintModeSame && remoteModeSame && teamSame;
}
