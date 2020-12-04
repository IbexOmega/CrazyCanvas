#pragma once

#include "ECS/System.h"

// If the local player is looking at a teammate further away than the maximum distance, the nameplate is not shown
#define MAX_NAMEPLATE_DISTANCE 75.0f

class HUDSystem;

class NameplateSystem : LambdaEngine::System
{
public:
	NameplateSystem(HUDSystem* pHUDSystem);
	~NameplateSystem() = default;

	void Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	LambdaEngine::IDVector m_ForeignPlayers;
	LambdaEngine::IDVector m_LocalPlayerCamera;

	uint8 m_LocalPlayerTeam = UINT8_MAX;

	HUDSystem* m_pHUDSystem;
};
