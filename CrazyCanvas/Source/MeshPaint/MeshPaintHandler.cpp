#include "MeshPaint/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderAPI.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"
#include "RenderStages/MeshPaintUpdater.h"

#include "Utilities/StringUtilities.h"

/*
* MeshPaintHandler
*/

LambdaEngine::TArray<MeshPaintHandler::UnwrapData> MeshPaintHandler::s_Collisions;

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

	m_pRenderGraph	= RenderSystem::GetInstance().GetRenderGraph();

	// Create buffer
	BufferDesc bufferDesc	= {};
	bufferDesc.DebugName	= "Mesh Paint Handler Points Buffer";
	bufferDesc.SizeInBytes	= sizeof(UnwrapData) * 10;
	bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
	bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
	m_pPointsBuffer			= RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);

	{
		byte* pBufferMapping = reinterpret_cast<byte*>(m_pPointsBuffer->Map());
		UnwrapData dummyData = {};
		dummyData.TargetPosition.w = 0.f;
		for (uint32 i = 0; i < 10; i++)
		{
			uint64 step = uint64(i) * sizeof(UnwrapData);
			memcpy(pBufferMapping + step, &dummyData, sizeof(UnwrapData));
		}
		m_pPointsBuffer->Unmap();

		Buffer* buf = m_pPointsBuffer.Get();
		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.ResourceName = "HIT_POINTS_BUFFER";
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer = &buf;
		resourceUpdateDesc.ExternalBufferUpdate.Count = 1;
		m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
	}
}

void MeshPaintHandler::Tick(LambdaEngine::Timestamp delta)
{
	using namespace LambdaEngine;

	UNREFERENCED_VARIABLE(delta);

	// To ensure the hit point is added in the main thread to the render graph
	// it is done in the tick function.

	bool transferMemory = false;

	// Reset memory if needed
	// if ((m_ResetPointBuffer && s_Collisions.IsEmpty()) || (m_PreviousPointsSize > s_Collisions.GetSize()))
	// {
	// 	byte* pBufferMapping = reinterpret_cast<byte*>(m_pPointsBuffer->Map());
	// 	UnwrapData dummyData = {};
	// 	dummyData.TargetPosition.w = 0.f;
	// 	for (uint32 i = 0; i < 10; i++)
	// 	{
	// 		uint64 step = uint64(i) * sizeof(UnwrapData);
	// 		memcpy(pBufferMapping + step, &dummyData, sizeof(UnwrapData));
	// 	}
	// 	m_pPointsBuffer->Unmap();

	// 	m_PreviousPointsSize = 0;
	// 	m_ResetPointBuffer = false;
	// 	transferMemory = true;
	// }

	// Load buffer with new data
	if (!s_Collisions.IsEmpty())
	{
		UnwrapData& dataFirst = s_Collisions.GetFront();
		dataFirst.TargetPosition.w = (float)s_Collisions.GetSize();

		if (s_ShouldReset)
		{
			UnwrapData& dataLast = s_Collisions.GetBack();
			dataLast.ClearClient = 1;
			s_ShouldReset = false;
		}

		LOG_WARNING("[TICK] s_Collisions size: %d, Copy data ...", s_Collisions.GetSize());
		byte* pBufferMapping = reinterpret_cast<byte*>(m_pPointsBuffer->Map());
		memcpy(pBufferMapping, s_Collisions.GetData(), s_Collisions.GetSize() * sizeof(UnwrapData));
		m_pPointsBuffer->Unmap();

		// TODO: Current implementation limits to 10 points and
		// discards the rest, not ideal but works for first implementation.
		m_PreviousPointsSize = s_Collisions.GetSize();
		s_Collisions.Clear();

		// When no new point is added to the list afterwards, make sure to zero the memory
		// to disable any further drawings
		m_ResetPointBuffer = true;
		transferMemory = true;
		MeshPaintUpdater::SetHitPointBufferValid(true);
	}

	// Transfer to GPU
	if (transferMemory)
	{
		LOG_WARNING("[TICK] UpdateResource");
		Buffer* buf = m_pPointsBuffer.Get();
		ResourceUpdateDesc resourceUpdateDesc				= {};
		resourceUpdateDesc.ResourceName						= "HIT_POINTS_BUFFER";
		resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &buf;
		resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
		m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
	}
}

void MeshPaintHandler::Release()
{
	m_pPointsBuffer.Reset();
}

void MeshPaintHandler::AddHitPoint(
	const glm::vec3& position,
	const glm::vec3& direction,
	EPaintMode paintMode,
	ERemoteMode remoteMode,
	ETeam team,
	uint32 angle)
{
	UnwrapData data = {};
	data.TargetPosition				= { position.x, position.y, position.z, 1.0f };
	data.TargetDirectionXYZAngleW	= { direction.x, direction.y, direction.z, glm::radians<float>((float)angle)};
	data.PaintMode			= paintMode;
	data.RemoteMode			= remoteMode;
	data.Team				= team;
	data.ClearClient		= 0;
	s_Collisions.PushBack(data);
}

void MeshPaintHandler::ResetClient()
{
	s_ShouldReset = true;
}

void MeshPaintHandler::ResetServer(LambdaEngine::Entity entity)
{
	using namespace LambdaEngine;

	MeshPaintUpdater::ClearServer(entity);
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
			AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team, projectileHitEvent.Angle);
			LOG_WARNING("[SERVER] Hit Pos: (%f, %f, %f), Dir: (%f, %f, %f), PaintMode: %s, RemoteMode: %s, Team: %d, Angle: %d",
				VEC_TO_ARG(collisionInfo.Position),
				VEC_TO_ARG(collisionInfo.Direction),
				PAINT_MODE_TO_STR(paintMode),
				REMOTE_MODE_TO_STR(remoteMode),
				team, projectileHitEvent.Angle);

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
			AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team, projectileHitEvent.Angle);
			LOG_WARNING("[CLIENT] Hit Pos: (%f, %f, %f), Dir: (%f, %f, %f), PaintMode: %s, RemoteMode: %s, Team: %d, Angle: %d",
				VEC_TO_ARG(collisionInfo.Position),
				VEC_TO_ARG(collisionInfo.Direction),
				PAINT_MODE_TO_STR(paintMode),
				REMOTE_MODE_TO_STR(remoteMode),
				team, projectileHitEvent.Angle);
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
		ResetClient();
		LOG_WARNING("[FROM SERVER] CLEAR CLIENT");

		// Allways paint the server's paint point to the server side mask (permanent mask)
		remoteMode = ERemoteMode::SERVER;
		AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team, packet.Angle);
		LOG_WARNING("[FROM SERVER] Hit Pos: (%f, %f, %f), Dir: (%f, %f, %f), PaintMode: %s, RemoteMode: %s, Team: %d, Angle: %d", 
			VEC_TO_ARG(packet.Position), 
			VEC_TO_ARG(packet.Direction),
			PAINT_MODE_TO_STR(paintMode),
			REMOTE_MODE_TO_STR(remoteMode),
			team, packet.Angle);
	}

	return true;
}
