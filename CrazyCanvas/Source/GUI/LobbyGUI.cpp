#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

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

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::KeyboardCallback);

}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::KeyboardCallback);
}

bool LobbyGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
{
	// ONLY FOR TESTING - REMOVE LATER
	if (event.Key == EKey::KEY_P)
	{
		AddSettingComboBox("maxTime", "max time", {"2 min", "5 min", "10 min"}, "2 min");
	}
	if (event.Key == EKey::KEY_O)
	{
		SetHostMode(!m_IsHost);
	}

	return false;
}

void LobbyGUI::AddPlayer(const LambdaEngine::String& playerName, uint32 team)
{
	StackPanel* pnl = team == 0 ? m_pBlueTeamStackPanel : m_pRedTeamStackPanel;

	// Grid
	Ptr<Grid> playerGrid = *new Grid();
	playerGrid->SetName((playerName + "_grid").c_str());
	RegisterName(playerName + "_grid", playerGrid);
	ColumnDefinitionCollection* columnCollection = playerGrid->GetColumnDefinitions();
	AddColumnDefinitionStar(columnCollection, 1.f);
	AddColumnDefinitionStar(columnCollection, 7.f);
	AddColumnDefinitionStar(columnCollection, 2.f);

	// Player label
	AddLabelWithStyle(playerName + "_name", playerGrid, "PlayerTeamLabelStyle", playerName);

	// Ping label
	AddLabelWithStyle(playerName + "_ping", playerGrid, "PingLabelStyle", "?");

	// Checkmark image
	Ptr<Image> image = *new Image();
	image->SetName((playerName + "_checkmark").c_str());
	RegisterName(playerName + "_checkmark", image);
	Style* style = FrameworkElement::FindResource<Style>("CheckmarkImageStyle");
	image->SetStyle(style);
	image->SetVisibility(Visibility::Visibility_Hidden);
	playerGrid->GetChildren()->Add(image);

	pnl->GetChildren()->Add(playerGrid);
}

void LobbyGUI::RemovePlayer(const LambdaEngine::String& playerName)
{
	Grid* grid = m_pBlueTeamStackPanel->FindName<Grid>((playerName + "_grid").c_str());
	if (grid)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(grid);
		return;
	}

	grid = m_pRedTeamStackPanel->FindName<Grid>((playerName + "_grid").c_str());
	if (grid)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(grid);
	}
}

void LobbyGUI::UpdatePlayerPing(const LambdaEngine::String& playerName, uint32 ping)
{
	Label* pingLabel = FrameworkElement::FindName<Label>((playerName + "_ping").c_str());
	if (pingLabel)
	{
		pingLabel->SetContent(std::to_string(ping).c_str());
	}
}

void LobbyGUI::UpdatePlayerReady(const LambdaEngine::String& playerName, bool ready)
{
	// Checkmark styling is currently broken
	Image* image = FrameworkElement::FindName<Image>((playerName + "_checkmark").c_str());
	if (image)
	{
		image->SetVisibility(ready ? Visibility::Visibility_Visible : Visibility::Visibility_Hidden);
	}
}

void LobbyGUI::WriteChatMessage(const LambdaEngine::String& playerName, const LambdaEngine::String& chatMessage)
{
	Ptr<DockPanel> dockPanel = *new DockPanel();

	AddLabelWithStyle("", dockPanel, "ChatNameLabelStyle", playerName);
	AddLabelWithStyle("", dockPanel, "ChatNameSeperatorStyle", "");

	Ptr<TextBox> message = *new TextBox();
	message->SetText(chatMessage.c_str());
	Style* style = FrameworkElement::FindResource<Style>("ChatMessageStyle");
	message->SetStyle(style);
	dockPanel->GetChildren()->Add(message);

	m_pChatPanel->GetChildren()->Add(dockPanel);
}

void LobbyGUI::SetLocalPlayerName(const LambdaEngine::String& playerName)
{
	m_LocalPlayerName = playerName;
}

void LobbyGUI::SetHostMode(bool isHost)
{
	m_IsHost = isHost;

	LOG_WARNING("Host is: %d", m_IsHost);

	m_pSettingsClientStackPanel->SetVisibility(m_IsHost ? Visibility_Hidden : Visibility_Visible);
	m_pSettingsHostStackPanel->SetVisibility(m_IsHost ? Visibility_Visible : Visibility_Hidden);
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

}

void LobbyGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{

}

void LobbyGUI::OnButtonSendMessageClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	LambdaEngine::String message = m_pChatInputTextBox->GetText();
	if (message == "")
		return;

	m_pChatInputTextBox->SetText("");
	WriteChatMessage(m_LocalPlayerName, message);
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
