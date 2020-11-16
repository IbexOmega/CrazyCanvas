#pragma once
#include "Game/State.h"

#include "ECS/System.h"

#include "Physics/PhysicsEvents.h"
#include "Events/GameplayEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/MatchEvents.h"
#include "Application/API/Events/WindowEvents.h"

#include "GUI/HUDGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"



class HUDSystem : public LambdaEngine::System
{
public:
	HUDSystem() = default;
	~HUDSystem();

	void Init();

	/*void Resume() override final {};
	void Pause() override final {};
	*/
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;
	void FixedTick(LambdaEngine::Timestamp delta);

	bool OnWeaponFired(const WeaponFiredEvent& event);
	bool OnWeaponReloadFinished(const WeaponReloadFinishedEvent& event);
	bool OnProjectileHit(const ProjectileHitEvent& event);
	bool OnPlayerScoreUpdated(const PlayerScoreUpdatedEvent& event);
	bool OnPlayerPingUpdated(const PlayerPingUpdatedEvent& event);
	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);
	bool OnGameOver(const GameOverEvent& event);
	bool OnWindowResized(const LambdaEngine::WindowResizedEvent& event);

private:
	bool OnMatchCountdownEvent(const MatchCountdownEvent& event);

private:
	LambdaEngine::IDVector m_PlayerEntities;
	LambdaEngine::IDVector m_ForeignPlayerEntities;
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_FlagEntities;
	LambdaEngine::IDVector m_CameraEntities;

	Noesis::Ptr<HUDGUI> m_HUDGUI;
	Noesis::Ptr<Noesis::IView> m_View;

	LambdaEngine::SpinLock m_DeferredEventsLock;
	LambdaEngine::TArray<ProjectileHitEvent> m_DeferredDamageTakenHitEvents;
	LambdaEngine::TArray<ProjectileHitEvent> m_DamageTakenEventsToProcess;

	LambdaEngine::TArray<bool> m_DeferredEnemyHitEvents;
	LambdaEngine::TArray<bool> m_EnemyHitEventsToProcess;
	
	uint32 m_LocalTeamIndex = UINT32_MAX;
};
