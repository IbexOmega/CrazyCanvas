#pragma once
#include "Game/State.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Events/PlayerEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"

#include "GUI/LobbyGUI.h"

class LobbyState : public LambdaEngine::State
{
public:
	LobbyState() = default;
	~LobbyState();

protected:
	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
	bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event);
	bool OnPlayerLeftEvent(const PlayerLeftEvent& event);
	bool OnPlayerInfoUpdatedEvent(const PlayerInfoUpdatedEvent& event);

	void SendServerConfiguration();

private:
	PacketGameSettings m_PacketGameSettings;

private:
	Noesis::Ptr<LobbyGUI> m_LobbyGUI;
	Noesis::Ptr<Noesis::IView> m_View;
};
