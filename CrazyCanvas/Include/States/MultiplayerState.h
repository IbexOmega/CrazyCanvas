#pragma once
#include "Game/State.h"

<<<<<<<< HEAD:CrazyCanvas/Include/States/MultiplayerState.h
#include "GUI/MultiplayerGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

// TEMP - DELTE IF PULLREQUEST
#include "GUI/LobbyGUI.h"
========
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Events/PlayerEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"
>>>>>>>> origin/lobby-gui:CrazyCanvas/Include/States/LobbyState.h

class MultiplayerState : public LambdaEngine::State
{
public:
	MultiplayerState() = default;
	~MultiplayerState();

protected:
	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override;
	void FixedTick(LambdaEngine::Timestamp delta) override;

private:
<<<<<<<< HEAD:CrazyCanvas/Include/States/MultiplayerState.h
	Noesis::Ptr<LobbyGUI> m_MultiplayerGUI;
	Noesis::Ptr<Noesis::IView> m_View;
========
	bool OnPlayerJoinedEvent(const PlayerJoinedEvent& event);
	bool OnPlayerLeftEvent(const PlayerLeftEvent& event);
	bool OnPlayerInfoUpdatedEvent(const PlayerInfoUpdatedEvent& event);

	void SendServerConfiguration();

private:
	PacketGameSettings m_PacketGameSettings;

private:
	/*Noesis::Ptr<LobbyGUI> m_LobbyGUI;
	Noesis::Ptr<Noesis::IView> m_View;*/
>>>>>>>> origin/lobby-gui:CrazyCanvas/Include/States/LobbyState.h
};
