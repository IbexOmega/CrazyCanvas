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

			PaintPoint paintPoint = {};
			paintPoint.Position		= collisionInfo.Position;
			paintPoint.Direction	= collisionInfo.Direction;
			paintPoint.PaintMode	= paintMode;
			paintPoint.RemoteMode	= remoteMode;
			paintPoint.Team			= team;
			paintPoint.Angle		= projectileHitEvent.Angle;
			m_PaintPointsOnClient.push(paintPoint);
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
		// We do not need to test the collisions against each other, because they will always be painted again but on the permanent mask. 
		// I still leave this here to be able to see if the client made a wrong prediction.
		bool clientWasWrong = false;
		if (m_PaintPointsOnClient.empty())
		{
			clientWasWrong = true;
			D_LOG_WARNING("Prediction Error: Client did not hit but server did, paint server's data to mask...");
		}
		else
		{
			PaintPoint paintPointA = m_PaintPointsOnClient.front();
			m_PaintPointsOnClient.pop();

			PaintPoint paintPointB = {};
			paintPointB.Position 	= packet.Position;
			paintPointB.Direction 	= packet.Direction;
			paintPointB.PaintMode 	= paintMode;
			paintPointB.RemoteMode 	= remoteMode;
			paintPointB.Team 		= team;
			paintPointB.Angle 		= packet.Angle;
			
#ifdef LAMBDA_DEBUG
			clientWasWrong = !IsPaintPointEqual(paintPointA, paintPointB);
			if(clientWasWrong)
			{	
				D_LOG_WARNING("Prediction Error: Client got wrong prediction when painting, reset client side paint and repaint on server side...");
			}
#endif
		}

		// Clear client side if all paint points have been processed.
		if (m_PaintPointsOnClient.empty())
		{
			PaintMaskRenderer::ResetClient();
		}
		
		// Allways paint the server's paint point to the server side mask (permanent mask)
		remoteMode = ERemoteMode::SERVER;
		PaintMaskRenderer::AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team, packet.Angle);
	}

	return true;
}

bool MeshPaintHandler::IsPaintPointEqual(PaintPoint& a, PaintPoint& b)
{
	bool posSame 		= glm::length2(a.Position - b.Position) < 0.0001f;
	bool dirSame 		= glm::length2(a.Direction - b.Direction) < 0.0001f;
	bool paintModeSame	= a.PaintMode == b.PaintMode;
	bool remoteModeSame = a.RemoteMode == b.RemoteMode;
	bool teamSame		= a.Team == b.Team;
	bool angleSame		= a.Angle == b.Angle;
	return posSame && dirSame && paintModeSame && remoteModeSame && teamSame && angleSame;
}
