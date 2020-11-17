#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/Grid.h"
#include "NsGui/Button.h"
#include "NsGui/StackPanel.h"
#include "NsGui/Panel.h"
#include "NsGui/TextBox.h"
#include "NsGui/Selector.h"
#include "NsGui/Label.h"

#include "Lobby/Player.h"
#include "Events/ChatEvents.h"

#include "Multiplayer/Packet/PacketGameSettings.h"

#include "Application/API/Events/KeyEvents.h"

class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI();
	~LobbyGUI();
	
	void InitGUI(LambdaEngine::String name);

	void AddPlayer(const Player& player);
	void RemovePlayer(const Player& player);
	void UpdatePlayerReady(const Player& player);
	void UpdatePlayerScore(const Player& player);
	void UpdatePlayerPing(const Player& player);
	void UpdatePlayerHost(const Player& player);
	void WriteChatMessage(const ChatEvent& event);
	void SetHostMode(bool isHost);
	void UpdateSettings(const PacketGameSettings& packet);

	void AddSettingComboBox(
		const LambdaEngine::String& settingKey,
		const LambdaEngine::String& settingText,
		LambdaEngine::TArray<LambdaEngine::String> settingValues,
		uint8 defaultIndex);

	void AddSettingTextBox(const LambdaEngine::String& settingKey, const LambdaEngine::String& settingText, const LambdaEngine::String& settingValue);

	// Noesis events
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;
	void OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnComboBoxSelectionChanged(Noesis::BaseComponent* pSender, const Noesis::SelectionChangedEventArgs& args);
	void OnTextBoxChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	const PacketGameSettings& GetSettings() const;

private:
	// Helpers
	void AddColumnDefinitionStar(Noesis::ColumnDefinitionCollection* pColumnCollection, float width);
	Noesis::Label* AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* pParent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content);
	void RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* pComp);
	void UnregisterName(const LambdaEngine::String& name);
	void CreateHostIcon(Noesis::Panel* pParent);
	Noesis::Grid* GetPlayerGrid(const Player& player);

	bool OnKeyPressedEvent(const LambdaEngine::KeyPressedEvent& event);
	void TrySendChatMessage();
	void SendGameSettings() const;

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid);

	Noesis::StackPanel* m_pBlueTeamStackPanel		= nullptr;
	Noesis::StackPanel* m_pRedTeamStackPanel		= nullptr;
	Noesis::ScrollViewer* m_pChatScrollViewer		= nullptr;
	Noesis::StackPanel* m_pChatPanel				= nullptr;
	Noesis::StackPanel* m_pSettingsNamesStackPanel	= nullptr;
	Noesis::StackPanel* m_pSettingsHostStackPanel	= nullptr;
	Noesis::StackPanel* m_pSettingsClientStackPanel	= nullptr;
	Noesis::TextBox*	m_pChatInputTextBox			= nullptr;

	PacketGameSettings m_GameSettings;
	bool m_IsInitiated;

private:
	static constexpr char* SETTING_SERVER_NAME	= "SERVER_NAME";
	static constexpr char* SETTING_MAP			= "MAP";
	static constexpr char* SETTING_MAX_TIME		= "MAX_TIME";
	static constexpr char* SETTING_FLAGS_TO_WIN = "FLAGS_TO_WIN";
	static constexpr char* SETTING_MAX_PLAYERS	= "MAX_PLAYERS";
	static constexpr char* SETTING_VISIBILITY	= "VISIBILITY";
	static constexpr char* SETTING_CHANGE_TEAM	= "CHANGE_TEAM";
};
