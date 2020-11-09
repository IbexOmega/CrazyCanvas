#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/Grid.h"
#include "NsGui/Button.h"
#include "NsGui/StackPanel.h"
#include "NsGui/Panel.h"
#include "NsGui/TextBox.h"

#include "Lobby/Player.h"
#include "Events/ChatEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"

#include "Application/API/Events/KeyEvents.h"

/*
* TODO:
*	- Different GUI when host
*	- Enable updating of game settings
*/

class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI();
	~LobbyGUI();

	void AddPlayer(const Player& player);
	void RemovePlayer(const Player& player);
	void UpdatePlayerReady(const Player& player);
	void UpdatePlayerPing(const Player& player);
	void WriteChatMessage(const ChatEvent& event);

	// Noesis events
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

private:
	// Helpers
	void AddColumnDefinitionStar(Noesis::ColumnDefinitionCollection* columnCollection, float width);
	void AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* parent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content);
	void RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* comp);

	void SendGameSettings();

	bool OnKeyPressedEvent(const LambdaEngine::KeyPressedEvent& event);
	void TrySendChatMessage();

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid);

	Noesis::StackPanel* m_pBlueTeamStackPanel	= nullptr;
	Noesis::StackPanel* m_pRedTeamStackPanel	= nullptr;
	Noesis::StackPanel* m_pChatPanel			= nullptr;
	Noesis::TextBox*	m_pChatInputTextBox		= nullptr;

	PacketGameSettings m_GameSettings;
};
