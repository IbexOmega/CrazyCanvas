#pragma once

#include "ECS/System.h"

#include "Application/API/Events/MouseEvents.h"
#include "Events/PlayerEvents.h"

#include "Containers/TStack.h"


/*
* SpectateCameraSystem
*/

class SpectateCameraSystem : public LambdaEngine::EntitySubscriber
{
public:
	SpectateCameraSystem() = default;
	~SpectateCameraSystem();
	
	void Init();

	bool OnMouseButtonClicked(const LambdaEngine::MouseButtonClickedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

private:
	void SpectatePlayer();

private:
	LambdaEngine::IDVector m_CameraEntities;
	uint8 m_LocalTeamIndex;

	int8 m_SpectatorIndex = 0;

	bool m_InSpectateView = false;

};