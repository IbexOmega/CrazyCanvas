#pragma once

#include "ECS/System.h"

// If the local player is looking at a teammate further away than the maximum distance, the nameplate is not shown
#define MAX_NAMEPLATE_DISTANCE 75.0f

class HUDSystem;

class NameplateSystem : LambdaEngine::EntitySubscriber
{
public:
	NameplateSystem(HUDSystem* pHUDSystem);
	~NameplateSystem() = default;

	void Init();

	void FixedTick(LambdaEngine::Timestamp deltaTime);

private:
	LambdaEngine::IDVector m_ForeignPlayers;
	LambdaEngine::IDVector m_LocalPlayerCamera;

	/*	If the local player looked at a teammate last frame, this will be set to that teammate's entity.
		Otherwise it's UINT32_MAX. */
	LambdaEngine::Entity m_PreviouslyViewedTeammate = UINT32_MAX;

	HUDSystem* m_pHUDSystem;
};
