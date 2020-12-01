#pragma once

#include "Application/API/Events/NetworkEvents.h"
#include "Events/PacketEvents.h"
#include "Game/State.h"
#include "ECS/Entity.h"
#include "ECS/Systems/Player/BenchmarkSystem.h"
#include "ECS/Systems/Player/WeaponSystem.h"
#include "EventHandlers/AudioEffectHandler.h"
#include "World/Level.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"

class Level;
struct WeaponFiredEvent;

class BenchmarkState : public LambdaEngine::State
{
public:
	BenchmarkState();
	~BenchmarkState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

private:
	static void PrintBenchmarkResults();

private:
	bool OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<PacketCreateLevelObject>& event);
	bool OnWeaponFired(const WeaponFiredEvent& event);

private:
	LambdaEngine::Entity m_Camera;
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[3];
	Level* m_pLevel = nullptr;

	/* Systems */
	BenchmarkSystem m_BenchmarkSystem;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
};
