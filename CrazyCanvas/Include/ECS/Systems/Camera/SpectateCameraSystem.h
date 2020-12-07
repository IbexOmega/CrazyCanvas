#pragma once

#include "ECS/System.h"

#include "Application/API/Events/MouseEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/MatchEvents.h"

#include "Containers/TStack.h"

#include "Types.h"


/*
* SpectateCameraSystem
*/

class SpectateCameraSystem : public LambdaEngine::System
{
public:
	SpectateCameraSystem() = default;
	~SpectateCameraSystem();
	
	void Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override;

	bool OnMouseButtonClicked(const LambdaEngine::MouseButtonClickedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);
	bool OnGameOver(const GameOverEvent& event);

private:
	void SpectatePlayer();

private:
	LambdaEngine::IDVector m_SpectatableEntities;
	LambdaEngine::IDVector m_CameraEntities;


	LambdaEngine::Entity m_SpectatedPlayer = UINT32_MAX;

	uint8 m_LocalTeamIndex;
	int8 m_SpectatorIndex = 0;

	bool m_InSpectateView = false;
	bool m_IsGameOver = false;

};