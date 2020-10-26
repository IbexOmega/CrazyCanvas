#pragma once
#include "ECS/Systems/Player/WeaponSystem.h"
#include "ECS/Systems/Player/HealthSystem.h"

#include "Game/State.h"

#include "ECS/Systems/GUI/HUDSystem.h"

#include "Application/API/Events/NetworkEvents.h"

#include "EventHandlers/AudioEffectHandler.h"
#include "EventHandlers/MeshPaintHandler.h"

#include "Networking/API/IPAddress.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState(LambdaEngine::IPAddress* pIPAddress);
	~PlaySessionState();

	void Init() override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
	void FixedTick(LambdaEngine::Timestamp delta) override final;

private:
	Level* m_pLevel = nullptr;

	LambdaEngine::IPAddress* m_pIPAddress;

	/* Systems */
	WeaponSystem m_WeaponSystem;
	HealthSystem m_HealthSystem;
	HUDSystem m_HUDSystem;

	/* Event handlers */
	AudioEffectHandler m_AudioEffectHandler;
	MeshPaintHandler m_MeshPaintHandler;
};
