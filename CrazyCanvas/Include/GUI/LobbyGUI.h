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
#include "Application/API/Events/MouseEvents.h"

class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI(PacketGameSettings* pGameSettings);
	~LobbyGUI();

	void InitGUI();

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

	void AddSettingColorBox(
		const LambdaEngine::String& settingKey,
		const LambdaEngine::String& settingText,
		const glm::vec3* pSettingColors, 
		uint32 numSettingColors,
		uint8 defaultIndex);

	void AddSettingTextBox(const LambdaEngine::String& settingKey, const LambdaEngine::String& settingText, const LambdaEngine::String& settingValue);

	// Noesis events
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;
	void OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnComboBoxSelectionChanged(Noesis::BaseComponent* pSender, const Noesis::SelectionChangedEventArgs& args);
	void OnTextBoxChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonChangeControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);
	void OnFOVSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);
	void OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplyControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);

private:
	// Helpers
	void AddColumnDefinitionStar(Noesis::ColumnDefinitionCollection* pColumnCollection, float width);
	Noesis::Label* AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* pParent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content);
	void AddTextBoxWithColor(const LambdaEngine::String& name, Noesis::Panel* pParent, const LambdaEngine::String& styleKey, const LambdaEngine::String& text, const glm::vec3& color);
	void RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* pComp);
	void UnregisterName(const LambdaEngine::String& name);
	void CreateHostIcon(Noesis::Panel* pParent);
	Noesis::Grid* GetPlayerGrid(const Player& player);

	bool OnKeyPressedEvent(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);
	void TrySendChatMessage();
	void SendGameSettings() const;
	void UpdatePlayersLabel();

	void SetDefaultSettings();
	void SetDefaultKeyBindings();

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid);

	Noesis::StackPanel*		m_pTeam1StackPanel			= nullptr;
	Noesis::StackPanel*		m_pTeam2StackPanel			= nullptr;
	Noesis::ScrollViewer*	m_pChatScrollViewer			= nullptr;
	Noesis::StackPanel*		m_pChatPanel				= nullptr;
	Noesis::StackPanel*		m_pSettingsNamesStackPanel	= nullptr;
	Noesis::StackPanel*		m_pSettingsHostStackPanel	= nullptr;
	Noesis::StackPanel*		m_pSettingsClientStackPanel	= nullptr;
	Noesis::TextBox*		m_pChatInputTextBox			= nullptr;
	Noesis::Label*			m_pPlayersLabel				= nullptr;
	Noesis::Label*			m_pTeam1Label				= nullptr;
	Noesis::Label*			m_pTeam2Label				= nullptr;
	Noesis::Grid*			m_pSettingsGrid				= nullptr;
	Noesis::Grid*			m_pControlsGrid				= nullptr;

	PacketGameSettings* m_pGameSettings;
	bool m_IsInitiated;

	bool			m_MeshShadersEnabled				= false;
	bool			m_FullscreenEnabled					= false;

	bool 			m_ListenToCallbacks = false;
	bool			m_MouseEnabled		= false;

	Noesis::Button* m_pSetKeyButton = nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;
	float32 m_LookSensitivityPercentageToSet = 0.0f;

private:
	static constexpr const char* SETTING_SERVER_NAME			= "SERVER_NAME";
	static constexpr const char* SETTING_MAP					= "MAP";
	static constexpr const char* SETTING_GAME_MODE				= "GAME_MODE";
	static constexpr const char* SETTING_MAX_TIME				= "MAX_TIME";
	static constexpr const char* SETTING_FLAGS_TO_WIN			= "FLAGS_TO_WIN";
	static constexpr const char* SETTING_MAX_PLAYERS			= "MAX_PLAYERS";
	static constexpr const char* SETTING_VISIBILITY				= "VISIBILITY";
	static constexpr const char* SETTING_CHANGE_TEAM			= "CHANGE_TEAM";
	static constexpr const char* SETTING_CHANGE_TEAM_1_COLOR	= "CHANGE_TEAM_1_COLOR";
	static constexpr const char* SETTING_CHANGE_TEAM_2_COLOR	= "CHANGE_TEAM_2_COLOR";
};
