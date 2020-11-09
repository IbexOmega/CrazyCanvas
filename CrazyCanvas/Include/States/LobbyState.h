#pragma once
#include "Game/State.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Events/PlayerEvents.h"
#include "Events/ChatEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"
#include "Multiplayer/Packet/PacketStartGame.h"

#include "GUI/LobbyGUI.h"

class LobbyState : public LambdaEngine::State
{
public:
	LobbyState(const LambdaEngine::String& name, bool isHost);
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
	bool OnPlayerStateUpdatedEvent(const PlayerStateUpdatedEvent& event);
	bool OnPlayerHostUpdatedEvent(const PlayerHostUpdatedEvent& event);
	bool OnPlayerPingUpdatedEvent(const PlayerPingUpdatedEvent& event);
	bool OnChatEvent(const ChatEvent& event);
	bool OnPacketStartGameReceived(const PacketReceivedEvent<PacketStartGame>& event);

private:
	Noesis::Ptr<LobbyGUI> m_LobbyGUI;
	Noesis::Ptr<Noesis::IView> m_View;
	LambdaEngine::String m_Name;
	bool m_IsHost;
};
