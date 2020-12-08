#include "MeshPaint/MeshPaintHandler.h"

#include "Application/API/Events/EventQueue.h"

#include "Game/Multiplayer/MultiplayerUtils.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderAPI.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/ServerHelper.h"
#include "RenderStages/MeshPaintUpdater.h"

#include "Utilities/StringUtilities.h"

/*
* MeshPaintHandler
*/

LambdaEngine::TArray<MeshPaintHandler::PaintData> MeshPaintHandler::s_Collisions;
LambdaEngine::SpinLock MeshPaintHandler::s_SpinLock;

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
	bufferDesc.SizeInBytes	= sizeof(PaintData) * 10;
	bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
	bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
	m_pPointsBuffer			= RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);

	{
		byte* pBufferMapping = reinterpret_cast<byte*>(m_pPointsBuffer->Map());
		PaintData dummyData = {};
		dummyData.TargetPosition.w = 0.f;
		for (uint32 i = 0; i < 10; i++)
		{
			uint64 step = uint64(i) * sizeof(PaintData);
			memcpy(pBufferMapping + step, &dummyData, sizeof(PaintData));
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

	// Load buffer with new data
	if (!s_Collisions.IsEmpty())
	{
		PaintData& dataFirst = s_Collisions.GetFront();
		dataFirst.TargetPosition.w = (float)s_Collisions.GetSize();

		if (s_ShouldReset)
		{
			PaintData& dataLast = s_Collisions.GetBack();
			dataLast.ClearClient = 1;
			s_ShouldReset = false;
		}

		byte* pBufferMapping = reinterpret_cast<byte*>(m_pPointsBuffer->Map());
		memcpy(pBufferMapping, s_Collisions.GetData(), s_Collisions.GetSize() * sizeof(PaintData));
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
	uint32 angle,
	LambdaEngine::Entity player,
	const glm::vec3& localPosition,
	const glm::vec3& localDirection)
{
	std::scoped_lock<LambdaEngine::SpinLock> lock(s_SpinLock);

	PaintData data = {};
	data.TargetPosition				= { position.x, position.y, position.z, 1.0f };
	data.TargetDirectionXYZAngleW	= { direction.x, direction.y, direction.z, glm::radians<float>((float)angle)};
	data.LocalPositionXYZPlayerW	= glm::vec4(localPosition, glm::uintBitsToFloat(player));
	data.LocalDirection				= glm::vec4(localDirection, 0.f);
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
		ETeam team = projectileHitEvent.Team;
		ERemoteMode remoteMode = ERemoteMode::UNDEFINED;

		if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			paintMode = EPaintMode::PAINT;
		}
		else if (projectileHitEvent.AmmoType == EAmmoType::AMMO_TYPE_WATER)
		{
			paintMode = EPaintMode::REMOVE;
		}

		ComponentArray<PlayerBaseComponent>* compArray = ECSCore::GetInstance()->GetComponentArray<PlayerBaseComponent>();
		Entity playerEntity = UINT32_MAX;
		bool isPlayer = true;
		if (compArray->HasComponent(projectileHitEvent.CollisionInfo0.Entity))
			playerEntity = projectileHitEvent.CollisionInfo0.Entity;
		else if (compArray->HasComponent(projectileHitEvent.CollisionInfo1.Entity))
			playerEntity = projectileHitEvent.CollisionInfo1.Entity;
		else
			isPlayer = false;

		PositionComponent posComp = {};
		ECSCore::GetInstance()->GetComponentIf<PositionComponent>(playerEntity, posComp);
		glm::vec3 playerPosition = posComp.Position;
		RotationComponent rotComp = {};
		ECSCore::GetInstance()->GetComponentIf<RotationComponent>(playerEntity, rotComp);
		glm::quat playerRotation = rotComp.Quaternion;

		glm::mat4 playerTransform = glm::toMat4(playerRotation);
		playerTransform = glm::translate(playerTransform, playerPosition);
		playerTransform = glm::inverse(playerTransform);
		
		const EntityCollisionInfo& collisionInfo = projectileHitEvent.CollisionInfo0;
		if (MultiplayerUtils::IsServer())
		{
			glm::vec4 projectilePos = playerTransform * glm::vec4(collisionInfo.Position, 1.f);
			glm::vec4 projectileDir = playerTransform * glm::vec4(collisionInfo.Direction, 0.f);

			remoteMode = ERemoteMode::SERVER;
			AddHitPoint(
				collisionInfo.Position,
				collisionInfo.Direction,
				paintMode,
				remoteMode,
				team,
				projectileHitEvent.Angle,
				isPlayer,
				projectilePos,
				projectileDir);
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
			packet.Position = collisionInfo.Position;
			packet.Direction = collisionInfo.Direction;
			packet.Angle = projectileHitEvent.Angle;
			if (isPlayer)
			{
				packet.LocalPosition = projectilePos;
				packet.LocalDirection = projectileDir;
				packet.IsPlayer = true;
			}
			ServerHelper::SendBroadcast(packet);
		}
		else
		{
			glm::vec4 projectilePos = playerTransform * glm::vec4(collisionInfo.Position, 1.f);
			glm::vec4 projectileDir = playerTransform * glm::vec4(collisionInfo.Direction, 0.f);

			// If it is a client, paint it on the temporary mask and save the point.
			remoteMode = ERemoteMode::CLIENT;
			AddHitPoint(collisionInfo.Position, collisionInfo.Direction, paintMode, remoteMode, team, projectileHitEvent.Angle, isPlayer, projectilePos, projectileDir);
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
		AddHitPoint(packet.Position, packet.Direction, paintMode, remoteMode, team, packet.Angle, packet.IsPlayer, packet.LocalPosition, packet.LocalDirection);
		LOG_WARNING("[FROM SERVER] Hit Pos: (%f, %f, %f), Dir: (%f, %f, %f), PaintMode: %s, RemoteMode: %s, Team: %d, Angle: %d",
			VEC_TO_ARG(packet.Position),
			VEC_TO_ARG(packet.Direction),
			PAINT_MODE_TO_STR(paintMode),
			REMOTE_MODE_TO_STR(remoteMode),
			team, packet.Angle);
	}

	return true;
}
