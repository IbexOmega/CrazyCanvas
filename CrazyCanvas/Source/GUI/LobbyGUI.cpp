#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Lobby/PlayerManagerClient.h"

#include "Containers/String.h"

#include "Game/StateManager.h"

#include "States/MultiplayerState.h"

#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/Packet/PacketStartGame.h"

#include "Application/API/Events/EventQueue.h"

using namespace Noesis;
using namespace LambdaEngine;

LobbyGUI::LobbyGUI()
{
	Noesis::GUI::LoadComponent(this, "Lobby.xaml");

	// Get commonly used elements
	m_pBlueTeamStackPanel		= FrameworkElement::FindName<StackPanel>("BlueTeamStackPanel");
	m_pRedTeamStackPanel		= FrameworkElement::FindName<StackPanel>("RedTeamStackPanel");
	m_pChatPanel				= FrameworkElement::FindName<StackPanel>("ChatStackPanel");
	m_pSettingsNamesStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsNamesStackPanel");
	m_pSettingsHostStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsClientStackPanel");
	m_pSettingsClientStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsHostStackPanel");
	m_pChatInputTextBox			= FrameworkElement::FindName<TextBox>("ChatInputTextBox");

	SetHostMode(false);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

void LobbyGUI::AddPlayer(const Player& player)
{
	StackPanel* pnl = player.GetTeam() == 0 ? m_pBlueTeamStackPanel : m_pRedTeamStackPanel;

	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	// Grid
	Ptr<Grid> playerGrid = *new Grid();
	playerGrid->SetName((uid + "_grid").c_str());
	RegisterName(uid + "_grid", playerGrid);
	ColumnDefinitionCollection* columnCollection = playerGrid->GetColumnDefinitions();
	AddColumnDefinitionStar(columnCollection, 1.f);
	AddColumnDefinitionStar(columnCollection, 7.f);
	AddColumnDefinitionStar(columnCollection, 2.f);

	// Player label
	AddLabelWithStyle(uid + "_name", playerGrid, "PlayerTeamLabelStyle", player.GetName());

	// Ping label
	AddLabelWithStyle(uid + "_ping", playerGrid, "PingLabelStyle", "-");

	// Checkmark image
	Ptr<Image> image = *new Image();
	image->SetName((uid + "_checkmark").c_str());
	RegisterName(uid + "_checkmark", image);
	Style* style = FrameworkElement::FindResource<Style>("CheckmarkImageStyle");
	image->SetStyle(style);
	image->SetVisibility(Visibility::Visibility_Hidden);
	playerGrid->GetChildren()->Add(image);

	pnl->GetChildren()->Add(playerGrid);
}

void LobbyGUI::RemovePlayer(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Grid* grid = m_pBlueTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (grid)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(grid);
		return;
	}

	grid = m_pRedTeamStackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (grid)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(grid);
	}
}

void LobbyGUI::UpdatePlayerPing(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Label* pingLabel = FrameworkElement::FindName<Label>((uid + "_ping").c_str());
	if (pingLabel)
	{
		pingLabel->SetContent(std::to_string(player.GetPing()).c_str());
	}
}

void LobbyGUI::UpdatePlayerHost(const Player& player)
{
	const Player* pPlayer = PlayerManagerClient::GetPlayerLocal();
	if (&player == pPlayer)
	{
		SetHostMode(player.IsHost());
	}
}

void LobbyGUI::UpdatePlayerReady(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	// Checkmark styling is currently broken
	Image* image = FrameworkElement::FindName<Image>((uid + "_checkmark").c_str());
	if (image)
	{
		image->SetVisibility(player.IsReady() ? Visibility::Visibility_Visible : Visibility::Visibility_Hidden);
	}
}

void LobbyGUI::WriteChatMessage(const ChatEvent& event)
{
	const ChatMessage& chatMessage = event.Message;

	const LambdaEngine::String& name = event.IsSystemMessage() ? "Server" : chatMessage.Name;

	Ptr<DockPanel> dockPanel = *new DockPanel();

	AddLabelWithStyle("", dockPanel, "ChatNameLabelStyle", name);
	AddLabelWithStyle("", dockPanel, "ChatNameSeperatorStyle", "");

	Ptr<TextBox> message = *new TextBox();
	message->SetText(chatMessage.Message.c_str());
	Style* style = FrameworkElement::FindResource<Style>("ChatMessageStyle");
	message->SetStyle(style);
	dockPanel->GetChildren()->Add(message);

	m_pChatPanel->GetChildren()->Add(dockPanel);
}

void LobbyGUI::SetHostMode(bool isHost)
{
	m_pSettingsClientStackPanel->SetVisibility(isHost ? Visibility_Hidden : Visibility_Visible);
	m_pSettingsHostStackPanel->SetVisibility(isHost ? Visibility_Visible : Visibility_Hidden);
}

void LobbyGUI::UpdateSetting(const LambdaEngine::String& settingKey, const LambdaEngine::String& value)
{
	Label* clientSetting = FrameworkElement::FindName<Label>((settingKey + "_client").c_str());
	clientSetting->SetContent(value.c_str());
}

void LobbyGUI::AddSettingComboBox(
	const LambdaEngine::String& settingKey,
	const LambdaEngine::String& settingText,
	TArray<LambdaEngine::String> settingValues,
	const std::string& defaultValue)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	AddLabelWithStyle(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientStyle", defaultValue);

	// Add setting combobox
	Ptr<ComboBox> settingComboBox = *new ComboBox();
	Style* style = FrameworkElement::FindResource<Style>("SettingsHostStyle");
	settingComboBox->SetStyle(style);
	settingComboBox->SetName((settingKey + "_host").c_str());
	settingComboBox->SelectionChanged() += MakeDelegate(this, &LobbyGUI::OnComboBoxSelectionChanged);
	RegisterName(settingComboBox->GetName(), settingComboBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingComboBox);

	for (auto& setting : settingValues)
	{
		Ptr<TextBlock> settingTextBlock = *new TextBlock();
		settingTextBlock->SetText(setting.c_str());
		settingComboBox->GetItems()->Add(settingTextBlock);
	}
	settingComboBox->SetSelectedIndex(0);
}

bool LobbyGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonReadyClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSendMessageClick);

	return false;
}

void LobbyGUI::OnButtonReadyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	const Player* pPlayer = PlayerManagerClient::GetPlayerLocal();

	if (pPlayer->IsHost())
	{
		PacketStartGame packet;
		ClientHelper::Send(packet);
	}
	else
	{
		PlayerManagerClient::SetLocalPlayerReady(!pPlayer->IsReady());
	}
}

void LobbyGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	ClientHelper::Disconnect("Leaving lobby");

	State* pMainMenuState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	TrySendChatMessage();
}

void LobbyGUI::SendGameSettings()
{
	ClientHelper::Send(m_GameSettings);
}

bool LobbyGUI::OnKeyPressedEvent(const KeyPressedEvent& event)
{
	if (event.Key == LambdaEngine::EKey::KEY_ENTER)
	{
		if (m_pChatInputTextBox->GetIsFocused())
		{
			TrySendChatMessage();
			return true;
		}
	}
	return false;
}

void LobbyGUI::TrySendChatMessage()
{
	LambdaEngine::String message = m_pChatInputTextBox->GetText();
	if (message.empty())
		return;

	m_pChatInputTextBox->SetText("");
	ChatManager::SendChatMessage(message);
}

void LobbyGUI::OnComboBoxSelectionChanged(Noesis::BaseComponent* pSender, const Noesis::SelectionChangedEventArgs& args)
{
	ComboBox* comboBox = static_cast<ComboBox*>(pSender);
	LOG_WARNING("Selected index: %d", comboBox->GetSelectedIndex());
	comboBox->SetText(static_cast<TextBlock*>(comboBox->GetSelectedItem())->GetText());

	// Update the local setting value to match
	size_t len = LambdaEngine::String(comboBox->GetName()).find_last_of("_");
	UpdateSetting(LambdaEngine::String(comboBox->GetName()).substr(0, len).c_str(), static_cast<TextBlock*>(comboBox->GetSelectedItem())->GetText());

	// If the value of the currently selected item is wanted, simple call the commented code below
	// LambdaEngine::String selectedValue = static_cast<TextBlock*>(comboBox->GetSelectedItem())->GetText();
}


void LobbyGUI::AddColumnDefinitionStar(ColumnDefinitionCollection* columnCollection, float width)
{
	GridLength gl = GridLength(width, GridUnitType::GridUnitType_Star);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	columnCollection->Add(col);
}

void LobbyGUI::AddLabelWithStyle(const LambdaEngine::String& name, Noesis::Panel* parent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content)
{
	Ptr<Label> label = *new Label();

	if (name != "")
	{
		label->SetName(name.c_str());
		RegisterName(name, label);
	}

	if (content != "")
		label->SetContent(content.c_str());

	Style* style = FrameworkElement::FindResource<Style>(styleKey.c_str());
	label->SetStyle(style);
	parent->GetChildren()->Add(label);
}

void LobbyGUI::RegisterName(const LambdaEngine::String& name, Noesis::BaseComponent* comp)
{
	FrameworkElement::GetView()->GetContent()->RegisterName(name.c_str(), comp);
}