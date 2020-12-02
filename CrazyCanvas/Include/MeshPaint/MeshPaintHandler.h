#pragma once
#include "Physics/PhysicsEvents.h"

#include "Multiplayer/Packet/PacketPlayerAliveChanged.h"
#include "Multiplayer/Packet/PacketProjectileHit.h"

#include "Events/PlayerEvents.h"
#include "Events/PacketEvents.h"
#include "Events/GameplayEvents.h"

#include "MeshPaintTypes.h"

#include <queue>

/*
* Helpers
*/

#define SET_TEAM_INDEX(mask, value) \
	mask |= (((uint8)value) & 0x0F)

#define SET_PAINT_MODE(mask, value) \
	mask |= ((((uint8)value) & 0x0F) << 4)

#define GET_TEAM_INDEX(mask) (ETeam)(mask & 0x0F)
#define GET_PAINT_MODE(mask) (EPaintMode)((mask & 0xF0) >> 4)

#define IS_MASK_PAINTED(mask)			(bool)(mask & 0x01)
#define GET_TEAM_INDEX_FROM_MASK(mask)	(ETeam)((mask >> 1) & 0x03)

namespace LambdaEngine
{
	class RenderGraph;
	class Buffer;
}

// MeshPaintHandler listens to events that should cause mesh to be painted
class MeshPaintHandler
{
public:
	MeshPaintHandler() = default;
	~MeshPaintHandler();

	void Init();
	void Tick(LambdaEngine::Timestamp delta);

public:
	/* Adds a hitpoint to draw out a splash at
	*	position	- vec3 of the hit point position
	*	direction	- vec3 of the direction the hit position had during collision
	*	paintMode	- painting mode to be used for the target
	*/
	static void AddHitPoint(const glm::vec3& position, const glm::vec3& direction, EPaintMode paintMode, ERemoteMode remoteMode, ETeam mode, uint32 angle);

	/* Reset client data from the texture and only use the verifed server data */
	static void ResetClient();

	/* Reset this entity's server texture (When player's are killed etc.) */
	static void ResetServer(LambdaEngine::Entity entity);

private:
	struct UnwrapData
	{
		glm::vec4		TargetPosition;
		glm::vec4		TargetDirectionXYZAngleW;
		EPaintMode		PaintMode			= EPaintMode::NONE;
		ERemoteMode		RemoteMode			= ERemoteMode::UNDEFINED;
		ETeam			Team				= ETeam::NONE;
		bool			ClearClient			= false;
	};

private:
	bool OnProjectileHit(const ProjectileHitEvent& projectileHitEvent);
	bool OnPacketProjectileHitReceived(const PacketReceivedEvent<PacketProjectileHit>& event);

private:
	LambdaEngine::RenderGraph* m_pRenderGraph = nullptr;

	LambdaEngine::TSharedRef<LambdaEngine::Buffer> m_pPointsBuffer;

	bool	m_ResetPointBuffer		= false;
	uint32	m_PreviousPointsSize	= 0;

private:
	static LambdaEngine::TArray<UnwrapData> s_Collisions;
	inline static bool	s_ShouldReset = false;
};
