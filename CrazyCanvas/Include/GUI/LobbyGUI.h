#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/Grid.h"
#include "NsGui/Button.h"
#include "NsGui/StackPanel.h"
#include "NsGui/Panel.h"
#include "NsGui/TextBox.h"
#include "NsGui/Selector.h"

// TEMP - REMOVE IF IT IS PULL REQUEST
#include "Application/API/Events/EventQueue.h"

class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI();
	~LobbyGUI();

	// TEMP - REMOVE IF IT IS PULL REQUEST
	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);

	void AddPlayer(const LambdaEngine::String& playerName, uint32 team);
	void RemovePlayer(const LambdaEngine::String& playerName);
	void UpdatePlayerReady(const LambdaEngine::String& playerName, bool ready);
	void UpdatePlayerPing(const LambdaEngine::String& playerName, uint32 ping);
	void WriteChatMessage(const LambdaEngine::String& playerName, const LambdaEngine::String& chatMessage);
	void SetLocalPlayerName(const LambdaEngine::String& playerName);
	void SetHostMode(bool isHost);
	void UpdateSetting(const LambdaEngine::String& settingKey, const LambdaEngine::String& value);

	void AddSettingComboBox(
		const LambdaEngine::String& settingKey,
		const LambdaEngine::String& settingText,
		LambdaEngine::TArray<LambdaEngine::String> settingValues,
		const std::string& defaultValue = "");

	// Noesis events
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;
	void OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnComboBoxSelectionChanged(Noesis::BaseComponent* pSender, const Noesis::SelectionChangedEventArgs& args);

private:
	// Helpers
	void AddColumnDefinitionStar(Noesis::ColumnDefinitionCollection* columnCollection, float width);
	void AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* parent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content);
	void RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* comp);

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid);

	Noesis::StackPanel* m_pBlueTeamStackPanel		= nullptr;
	Noesis::StackPanel* m_pRedTeamStackPanel		= nullptr;
	Noesis::StackPanel* m_pChatPanel				= nullptr;
	Noesis::StackPanel* m_pSettingsNamesStackPanel	= nullptr;
	Noesis::StackPanel* m_pSettingsHostStackPanel	= nullptr;
	Noesis::StackPanel* m_pSettingsClientStackPanel	= nullptr;
	Noesis::TextBox*	m_pChatInputTextBox			= nullptr;

	LambdaEngine::String m_LocalPlayerName		= "";
	bool m_IsHost								= false;
};