#pragma once

#include "ECS/System.h"

#include "Application/API/Events/MouseEvents.h"
#include "Events/PlayerEvents.h"

#include "Containers/TStack.h"


/*
* CameraSystem
*/

class Player;

class CameraChangeSystem : public LambdaEngine::System
{
public:
	CameraChangeSystem() = default;
	~CameraChangeSystem();
	
	void Init();

	void Tick(LambdaEngine::Timestamp deltaTime);
	void FixedTick(LambdaEngine::Timestamp deltaTime);

	bool OnMouseButtonClicked(const LambdaEngine::MouseButtonClickedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

protected:
	LambdaEngine::IDVector m_PlayerEntities;
	LambdaEngine::IDVector m_CameraEntities;

private:
	uint8 m_LocalTeamIndex;

	uint8 m_SpectatorIndex = 0;

	LambdaEngine::TArray<LambdaEngine::Entity> m_SpectatorStack;
	LambdaEngine::TArray<const Player*> m_TeamPlayers;

};