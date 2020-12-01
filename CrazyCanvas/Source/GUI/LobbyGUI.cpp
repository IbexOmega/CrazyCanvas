#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Lobby/PlayerManagerClient.h"

#include "Containers/String.h"

#include "Game/StateManager.h"

#include "States/MultiplayerState.h"

#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "World/LevelManager.h"

#include "Teams/TeamHelper.h"

using namespace Noesis;
using namespace LambdaEngine;

LobbyGUI::LobbyGUI(PacketGameSettings* pGameSettings) :
	m_IsInitiated(false),
	m_pGameSettings(pGameSettings)
{
	GUI::LoadComponent(this, "Lobby.xaml");

	// Get commonly used elements
	m_pTeam1StackPanel			= FrameworkElement::FindName<StackPanel>("Team1StackPanel");
	m_pTeam2StackPanel			= FrameworkElement::FindName<StackPanel>("Team2StackPanel");
	m_pTeam1Label				= FrameworkElement::FindName<Label>("Team1Label");
	m_pTeam2Label				= FrameworkElement::FindName<Label>("Team2Label");
	m_pChatScrollViewer			= FrameworkElement::FindName<ScrollViewer>("ChatScrollViewer");
	m_pChatPanel				= FrameworkElement::FindName<StackPanel>("ChatStackPanel");
	m_pSettingsNamesStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsNamesStackPanel");
	m_pSettingsHostStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsClientStackPanel");
	m_pSettingsClientStackPanel	= FrameworkElement::FindName<StackPanel>("SettingsHostStackPanel");
	m_pChatInputTextBox			= FrameworkElement::FindName<TextBox>("ChatInputTextBox");
	m_pPlayersLabel				= FrameworkElement::FindName<Label>("PlayersLabel");

	m_pChatInputTextBox->SetMaxLines(1);
	m_pChatInputTextBox->SetMaxLength(128);

	SetHostMode(false);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

LobbyGUI::~LobbyGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &LobbyGUI::OnKeyPressedEvent);
}

void LobbyGUI::InitGUI()
{
	TArray<LambdaEngine::String> gameModeNames;
	TArray<EGameMode> gameModes;
	GameModesQuery(gameModes);
	gameModeNames.Reserve(gameModes.GetSize());
	for (EGameMode gameMode : gameModes)
		gameModeNames.PushBack(GameModeToString(gameMode));

	TArray<glm::vec3> colors = TeamHelper::GetAllAvailableColors();

	AddSettingTextBox(SETTING_SERVER_NAME,      "Server Name",			m_pGameSettings->ServerName);
	AddSettingComboBox(SETTING_MAP,				"Map",					LevelManager::GetLevelNames(), 0);
	AddSettingComboBox(SETTING_GAME_MODE,		"Game Mode",			gameModeNames, (uint8)m_pGameSettings->GameMode);
	AddSettingComboBox(SETTING_FLAGS_TO_WIN,	"Flags To Win",			{ "3", "5", "10", "15" }, 1);
	AddSettingComboBox(SETTING_MAX_PLAYERS,		"Max Players",			{ "4", "6", "8", "10" }, 3);
	/*AddSettingComboBox(SETTING_MAX_TIME,		"Max Time",				{ "3 min", "5 min", "10 min", "15 min" }, 1);
	AddSettingComboBox(SETTING_VISIBILITY,		"Visibility",			{ "True", "False" }, 1);
	AddSettingComboBox(SETTING_CHANGE_TEAM,		"Allow Change Team",	{ "True", "False" }, 1);*/
	AddSettingColorBox(SETTING_CHANGE_TEAM_1_COLOR, "Team 1 Color", colors, 0);
	AddSettingColorBox(SETTING_CHANGE_TEAM_2_COLOR, "Team 2 Color", colors, 1);

	UpdateSettings(*m_pGameSettings);

	m_IsInitiated = true;
}

void LobbyGUI::AddPlayer(const Player& player)
{
	StackPanel* pPanel = player.GetTeam() == 1 ? m_pTeam1StackPanel : m_pTeam2StackPanel;

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
	Style* pStyle = FrameworkElement::FindResource<Style>("CheckmarkImageStyle");
	image->SetStyle(pStyle);
	image->SetVisibility(Visibility::Visibility_Hidden);
	playerGrid->GetChildren()->Add(image);

	pPanel->GetChildren()->Add(playerGrid);

	UpdatePlayersLabel();
}

void LobbyGUI::RemovePlayer(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());
	const LambdaEngine::String& uidGrid = uid + "_grid";

	Grid* pGrid = FrameworkElement::FindName<Grid>(uidGrid.c_str());

	if (m_pTeam1StackPanel->GetChildren()->Contains(pGrid))
	{
		m_pTeam1StackPanel->GetChildren()->Remove(pGrid);
	}
	else
	{
		m_pTeam2StackPanel->GetChildren()->Remove(pGrid);
	}

	UnregisterName(uidGrid);
	UnregisterName(uid + "_checkmark");
	UnregisterName(uid + "_name");
	UnregisterName(uid + "_ping");

	if (player.IsHost())
		UnregisterName("host_icon");

	UpdatePlayersLabel();
}

void LobbyGUI::UpdatePlayerPing(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Label* pPingLabel = FrameworkElement::FindName<Label>((uid + "_ping").c_str());
	if (pPingLabel)
	{
		pPingLabel->SetContent(std::to_string(player.GetPing()).c_str());
	}
}

void LobbyGUI::UpdatePlayerHost(const Player& player)
{
	const Player* pPlayer = PlayerManagerClient::GetPlayerLocal();
	if (&player == pPlayer)
	{
		SetHostMode(player.IsHost());
	}

	// Set host icon
	if (player.IsHost())
	{
		// Host icon is not an image due to AA problems, it is a vector path instead
		Viewbox* crownBox = FrameworkElement::FindName<Viewbox>("host_icon");
		if (crownBox)
		{
			Grid* newGrid = GetPlayerGrid(player);
			Grid* oldGrid = static_cast<Grid*>(crownBox->GetParent());
			oldGrid->GetChildren()->Remove(crownBox);
			newGrid->GetChildren()->Add(crownBox);
		}
		else
		{
			// CreateHostIcon()
			Grid* grid = GetPlayerGrid(player);
			if (grid)
			{
				CreateHostIcon(grid);
			}
			else
			{
				LOG_WARNING("Player %s could not be found when updating player host!", player.GetName().c_str());
			}
		}
	}
}

void LobbyGUI::UpdatePlayerReady(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	// Checkmark styling is currently broken
	Image* pImage = FrameworkElement::FindName<Image>((uid + "_checkmark").c_str());
	if (pImage)
	{
		pImage->SetVisibility(player.IsReady() ? Visibility::Visibility_Visible : Visibility::Visibility_Hidden);
	}
}

void LobbyGUI::UpdatePlayerScore(const Player& player)
{
	RemovePlayer(player);
	AddPlayer(player);
	UpdatePlayerHost(player);
}

void LobbyGUI::WriteChatMessage(const ChatEvent& event)
{
	const ChatMessage& chatMessage = event.Message;

	const LambdaEngine::String& name = event.IsSystemMessage() ? "Server" : chatMessage.Name;

	Ptr<DockPanel> dockPanel = *new DockPanel();

	Label* pLabel = AddLabelWithStyle("", dockPanel, "ChatNameLabelStyle", name);
	AddLabelWithStyle("", dockPanel, "ChatNameSeperatorStyle", "");

	Ptr<SolidColorBrush> pBrush = *new SolidColorBrush();
	if (event.IsSystemMessage())
	{
		pBrush->SetColor(Color::Green());
	}
	else
	{
		uint8 colorIndex = chatMessage.Team == 1 ? m_pGameSettings->TeamColor1 : m_pGameSettings->TeamColor2;
		glm::vec3 teamColor = TeamHelper::GetAvailableColor(colorIndex);
		Color chatMessageColor(teamColor.r, teamColor.g, teamColor.b);

		pBrush->SetColor(chatMessageColor);
	}
	pLabel->SetForeground(pBrush);

	Ptr<Label> message = *new Label();
	message->SetFocusable(false);
	message->SetContent(chatMessage.Message.c_str());
	message->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Center);
	message->SetPadding(Thickness(3, 0, 0, 0));
	dockPanel->GetChildren()->Add(message);

	m_pChatPanel->GetChildren()->Add(dockPanel);
	m_pChatScrollViewer->ScrollToEnd();
}

void LobbyGUI::SetHostMode(bool isHost)
{
	Button* pReadyButton = FrameworkElement::FindName<Button>("ReadyButton");

	if (isHost)
	{
		pReadyButton->SetContent("Start");
		m_pSettingsClientStackPanel->SetVisibility(Visibility_Hidden);
		m_pSettingsHostStackPanel->SetVisibility(Visibility_Visible);
		SendGameSettings();
	}
	else
	{
		pReadyButton->SetContent("Ready");
		m_pSettingsClientStackPanel->SetVisibility(Visibility_Visible);
		m_pSettingsHostStackPanel->SetVisibility(Visibility_Hidden);
	}
}

void LobbyGUI::UpdateSettings(const PacketGameSettings& packet)
{
	*m_pGameSettings = packet;

	Label* pSettingServerName = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_SERVER_NAME) + "_client").c_str());
	if(pSettingServerName)
		pSettingServerName->SetContent(packet.ServerName);

	Label* pSettingMap = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAP) + "_client").c_str());
	if (pSettingMap)
		pSettingMap->SetContent(LevelManager::GetLevelNames()[packet.MapID].c_str());

	Label* pSettingGameMode = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_GAME_MODE) + "_client").c_str());
	if (pSettingGameMode)
		pSettingGameMode->SetContent(GameModeToString(packet.GameMode));

	Label* pSettingMaxTime = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAX_TIME) + "_client").c_str());
	if (pSettingMaxTime)
		pSettingMaxTime->SetContent((std::to_string(packet.MaxTime / 60) + " min").c_str());

	Label* pSettingFlagsToWin = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_FLAGS_TO_WIN) + "_client").c_str());
	if (pSettingFlagsToWin)
		pSettingFlagsToWin->SetContent(std::to_string(packet.FlagsToWin).c_str());

	Label* pSettingMaxPlayers = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_MAX_PLAYERS) + "_client").c_str());
	if (pSettingMaxPlayers)
		pSettingMaxPlayers->SetContent(std::to_string(packet.Players).c_str());

	Label* pSettingVisibility = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_VISIBILITY) + "_client").c_str());
	if (pSettingVisibility)
		pSettingVisibility->SetContent(packet.Visible ? "True" : "False");

	Label* pSettingChangeTeam = FrameworkElement::FindName<Label>((LambdaEngine::String(SETTING_CHANGE_TEAM) + "_client").c_str());
	if (pSettingChangeTeam)
		pSettingChangeTeam->SetContent(packet.ChangeTeam ? "True" : "False");

	TextBlock* pSettingChangeTeamColor1 = FrameworkElement::FindName<TextBlock>((LambdaEngine::String(SETTING_CHANGE_TEAM_1_COLOR) + "_client").c_str());
	if (pSettingChangeTeamColor1)
	{
		glm::vec3 color = TeamHelper::GetAvailableColor((uint8)packet.TeamColor1);
		Color teamColor = Color(color.r, color.g, color.b);

		// Update Settings Color
		SolidColorBrush* pSolidColorBrush = static_cast<SolidColorBrush*>(pSettingChangeTeamColor1->GetBackground());
		pSolidColorBrush->SetColor(teamColor);
		
		// Update Team Label color
		pSolidColorBrush = static_cast<SolidColorBrush*>(m_pTeam1Label->GetForeground());
		pSolidColorBrush->SetColor(teamColor);

		// Update old messages text color
		m_pChatPanel->GetChildren()->Clear();
		ChatManager::RenotifyAllChatMessages();
	}

	TextBlock* pSettingChangeTeamColor2 = FrameworkElement::FindName<TextBlock>((LambdaEngine::String(SETTING_CHANGE_TEAM_2_COLOR) + "_client").c_str());
	if (pSettingChangeTeamColor2)
	{
		glm::vec3 color = TeamHelper::GetAvailableColor((uint8)packet.TeamColor2);
		Color teamColor = Color(color.r, color.g, color.b);

		// Update Settings Color
		SolidColorBrush* pSolidColorBrush = static_cast<SolidColorBrush*>(pSettingChangeTeamColor2->GetBackground());
		pSolidColorBrush->SetColor(teamColor);

		// Update Team Label color
		pSolidColorBrush = static_cast<SolidColorBrush*>(m_pTeam2Label->GetForeground());
		pSolidColorBrush->SetColor(teamColor);

		// Update old messages text color
		m_pChatPanel->GetChildren()->Clear();
		ChatManager::RenotifyAllChatMessages();
	}


	TextBox* pServerNameTextBox = FrameworkElement::FindName<TextBox>((LambdaEngine::String(SETTING_SERVER_NAME) + "_host").c_str());
	if (pServerNameTextBox)
		pServerNameTextBox->SetText(packet.ServerName);

	ComboBox* pSettingMapHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_MAP) + "_host").c_str());
	if (pSettingMapHost)
		pSettingMapHost->SetSelectedIndex(packet.MapID);

	ComboBox* pSettingGameModeHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_GAME_MODE) + "_host").c_str());
	if (pSettingGameModeHost)
		pSettingGameModeHost->SetSelectedIndex((uint8)packet.GameMode);

	ComboBox* pSettingMaxTimeHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_MAX_TIME) + "_host").c_str());
	if (pSettingMaxTimeHost)
	{
		int minutes = packet.MaxTime / 60;
		if(minutes == 3)
			pSettingMaxTimeHost->SetSelectedIndex(0);
		else if (minutes == 5)
			pSettingMaxTimeHost->SetSelectedIndex(1);
		else if (minutes == 10)
			pSettingMaxTimeHost->SetSelectedIndex(2);
		else if (minutes == 15)
			pSettingMaxTimeHost->SetSelectedIndex(3);
	}
		

	ComboBox* pSettingFlagsToWinHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_FLAGS_TO_WIN) + "_host").c_str());
	if (pSettingFlagsToWinHost)
	{
		int flags = packet.FlagsToWin;
		if (flags == 3)
			pSettingFlagsToWinHost->SetSelectedIndex(0);
		else if (flags == 5)
			pSettingFlagsToWinHost->SetSelectedIndex(1);
		else if (flags == 10)
			pSettingFlagsToWinHost->SetSelectedIndex(2);
		else if (flags == 15)
			pSettingFlagsToWinHost->SetSelectedIndex(3);
	}

	ComboBox* pSettingMaxPlayersHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_MAX_PLAYERS) + "_host").c_str());
	if (pSettingMaxPlayersHost)
	{
		int players = packet.Players;
		if (players == 4)
			pSettingMaxPlayersHost->SetSelectedIndex(0);
		else if (players == 6)
			pSettingMaxPlayersHost->SetSelectedIndex(1);
		else if (players == 8)
			pSettingMaxPlayersHost->SetSelectedIndex(2);
		else if (players == 10)
			pSettingMaxPlayersHost->SetSelectedIndex(3);
	}

	ComboBox* pSettingVisibilityHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_VISIBILITY) + "_host").c_str());
	if (pSettingVisibilityHost)
		pSettingVisibilityHost->SetSelectedIndex(packet.Visible ? 0 : 1);

	ComboBox* pSettingChangeTeamHost = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_CHANGE_TEAM) + "_host").c_str());
	if (pSettingChangeTeamHost)
		pSettingChangeTeamHost->SetSelectedIndex(packet.ChangeTeam ? 0 : 1);

	ComboBox* pSettingTeam1Color = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_CHANGE_TEAM_1_COLOR) + "_host").c_str());
	if (pSettingTeam1Color)
	{
		pSettingTeam1Color->SetSelectedIndex(packet.TeamColor1 - 1);
		TextBlock* pBox = (TextBlock*)pSettingTeam1Color->GetSelectedItem();
		pSettingTeam1Color->SetBackground(pBox->GetBackground());
	}

	ComboBox* pSettingTeam2Color = FrameworkElement::FindName<ComboBox>((LambdaEngine::String(SETTING_CHANGE_TEAM_2_COLOR) + "_host").c_str());
	if (pSettingTeam2Color)
	{
		pSettingTeam2Color->SetSelectedIndex(packet.TeamColor2 - 1);
		TextBlock* pBox = (TextBlock*)pSettingTeam2Color->GetSelectedItem();
		pSettingTeam2Color->SetBackground(pBox->GetBackground());
	}

	UpdatePlayersLabel();
}

void LobbyGUI::AddSettingComboBox(
	const LambdaEngine::String& settingKey,
	const LambdaEngine::String& settingText,
	TArray<LambdaEngine::String> settingValues,
	uint8 defaultIndex)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	AddLabelWithStyle(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientStyle", settingValues[defaultIndex]);

	// Add setting combobox
	Ptr<ComboBox> settingComboBox = *new ComboBox();
	Style* pStyle = FrameworkElement::FindResource<Style>("SettingsHostStyle");
	settingComboBox->SetStyle(pStyle);
	settingComboBox->SetName((settingKey + "_host").c_str());
	settingComboBox->SelectionChanged() += MakeDelegate(this, &LobbyGUI::OnComboBoxSelectionChanged);
	settingComboBox->SetFocusable(false);
	RegisterName(settingComboBox->GetName(), settingComboBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingComboBox);

	for (auto& setting : settingValues)
	{
		Ptr<TextBlock> settingTextBlock = *new TextBlock();
		settingTextBlock->SetText(setting.c_str());
		settingComboBox->GetItems()->Add(settingTextBlock);
	}
	settingComboBox->SetSelectedIndex(defaultIndex);
}

void LobbyGUI::AddSettingColorBox(const LambdaEngine::String& settingKey, const LambdaEngine::String& settingText, const LambdaEngine::TArray<glm::vec3>& settingColors, uint8 defaultIndex)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	// Temporary until Rectangle can be adjusted to fit parent
	AddTextBoxWithColor(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientTextStyle", "\t\t\t\t", TeamHelper::GetAvailableColor(defaultIndex + 1));

	// Add setting combobox for colors
	Ptr<ComboBox> settingComboBox = *new ComboBox();
	Style* pStyle = FrameworkElement::FindResource<Style>("SettingsHostStyle");
	settingComboBox->SetStyle(pStyle);
	settingComboBox->SetName((settingKey + "_host").c_str());
	settingComboBox->SelectionChanged() += MakeDelegate(this, &LobbyGUI::OnComboBoxSelectionChanged);
	settingComboBox->SetFocusable(false);
	RegisterName(settingComboBox->GetName(), settingComboBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingComboBox);

	for (auto& color : settingColors)
	{
		Ptr<SolidColorBrush> pBrush = *new SolidColorBrush();
		pBrush->SetColor(Color(color.r, color.g, color.b));

		Ptr<TextBlock> settingTextBlock = *new TextBlock();
		settingTextBlock->SetText("");
		settingTextBlock->SetBackground(pBrush);
		settingComboBox->GetItems()->Add(settingTextBlock);
	}
	settingComboBox->SetSelectedIndex(defaultIndex);
}

void LobbyGUI::AddSettingTextBox(
	const LambdaEngine::String& settingKey,
	const LambdaEngine::String& settingText,
	const LambdaEngine::String& settingValue)
{
	// Add setting text
	AddLabelWithStyle("", m_pSettingsNamesStackPanel, "SettingsNameStyle", settingText);

	// Add setting client text (default value is set as content)
	AddLabelWithStyle(settingKey + "_client", m_pSettingsClientStackPanel, "SettingsClientStyle", settingText);

	// Add setting textbox
	Ptr<TextBox> settingTextBox = *new TextBox();
	Style* pStyle = FrameworkElement::FindResource<Style>("SettingsHostTextStyle");
	settingTextBox->SetStyle(pStyle);
	settingTextBox->SetName((settingKey + "_host").c_str());
	settingTextBox->TextChanged() += MakeDelegate(this, &LobbyGUI::OnTextBoxChanged);
	settingTextBox->SetMaxLength(MAX_NAME_LENGTH - 1);
	settingTextBox->SetMaxLines(1);
	settingTextBox->SetText(settingValue.substr(0, glm::min<int32>((int32)settingValue.length(), settingTextBox->GetMaxLength())).c_str());
	RegisterName(settingTextBox->GetName(), settingTextBox);
	m_pSettingsHostStackPanel->GetChildren()->Add(settingTextBox);
}


bool LobbyGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonReadyClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSendMessageClick);

	return false;
}

void LobbyGUI::OnButtonReadyClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	ToggleButton* pButton = static_cast<ToggleButton*>(pSender);
	PlayerManagerClient::SetLocalPlayerReady(pButton->GetIsChecked().GetValue());
}

void LobbyGUI::OnButtonLeaveClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	ClientHelper::Disconnect("Leaving lobby");

	State* pMainMenuState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void LobbyGUI::OnButtonSendMessageClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	TrySendChatMessage();
}

bool LobbyGUI::OnKeyPressedEvent(const KeyPressedEvent& event)
{
	if (event.Key == EKey::KEY_ENTER)
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

void LobbyGUI::SendGameSettings() const
{
	if (m_IsInitiated)
		ClientHelper::Send(*m_pGameSettings);
}

void LobbyGUI::UpdatePlayersLabel()
{
	const THashTable<uint64, Player>& players = PlayerManagerClient::GetPlayers();
	m_pPlayersLabel->SetContent((std::to_string(players.size()) + "/" + std::to_string(m_pGameSettings->Players) + " Players").c_str());
}

void LobbyGUI::OnComboBoxSelectionChanged(BaseComponent* pSender, const SelectionChangedEventArgs& args)
{
	if (!m_IsInitiated)
		return;

	ComboBox* pComboBox = static_cast<ComboBox*>(pSender);

	LambdaEngine::String setting = pComboBox->GetName();
	setting = setting.substr(0, setting.find_last_of("_"));
	uint32 indexSelected = pComboBox->GetSelectedIndex();
	LambdaEngine::String textSelected = static_cast<TextBlock*>(pComboBox->GetSelectedItem())->GetText();

	if (setting == SETTING_MAP)
	{
		m_pGameSettings->MapID = (uint8)indexSelected;
	}
	else if (setting == SETTING_GAME_MODE)
	{
		m_pGameSettings->GameMode = GameModeParseString(textSelected.c_str());
	}
	else if (setting == SETTING_MAX_TIME)
	{
		textSelected = textSelected.substr(0, textSelected.find_last_of(" min"));
		m_pGameSettings->MaxTime = (uint16)std::stoi(textSelected) * 60;
	}
	else if (setting == SETTING_FLAGS_TO_WIN)
	{
		m_pGameSettings->FlagsToWin = (uint8)std::stoi(textSelected);
	}
	else if (setting == SETTING_MAX_PLAYERS)
	{
		m_pGameSettings->Players = (uint8)std::stoi(textSelected);
		UpdatePlayersLabel();
	}
	else if (setting == SETTING_VISIBILITY)
	{
		m_pGameSettings->Visible = textSelected == "True";
	}
	else if (setting == SETTING_CHANGE_TEAM)
	{
		m_pGameSettings->ChangeTeam = textSelected == "True";
	}
	else if (setting == SETTING_CHANGE_TEAM_1_COLOR || setting == SETTING_CHANGE_TEAM_2_COLOR)
	{
		TextBlock* pBox = (TextBlock*)pComboBox->GetSelectedItem();
		SolidColorBrush* pBoxColorBrush = static_cast<SolidColorBrush*>(pBox->GetBackground());
		pComboBox->SetBackground(pBoxColorBrush);

		SolidColorBrush* pLabelColorBrush = nullptr;

		if (setting == SETTING_CHANGE_TEAM_1_COLOR)
		{
			pLabelColorBrush = static_cast<SolidColorBrush*>(m_pTeam1Label->GetForeground());
			m_pGameSettings->TeamColor1 = (uint8)indexSelected + 1;
		}
		else
		{
			pLabelColorBrush = static_cast<SolidColorBrush*>(m_pTeam2Label->GetForeground());
			m_pGameSettings->TeamColor2 = (uint8)indexSelected + 1;
		}

		pLabelColorBrush->SetColor(pBoxColorBrush->GetColor());
			
		m_pChatPanel->GetChildren()->Clear();
		ChatManager::RenotifyAllChatMessages();
	}

	SendGameSettings();
}

void LobbyGUI::OnTextBoxChanged(BaseComponent* pSender, const RoutedEventArgs& args)
{
	TextBox* pTextBox = static_cast<TextBox*>(pSender);

	if (strcmp(m_pGameSettings->ServerName, pTextBox->GetText()) != 0) 
	{
		strcpy(m_pGameSettings->ServerName, pTextBox->GetText());

		SendGameSettings();
	}
}

void LobbyGUI::AddColumnDefinitionStar(ColumnDefinitionCollection* columnCollection, float width)
{
	GridLength gl = GridLength(width, GridUnitType::GridUnitType_Star);
	Ptr<ColumnDefinition> col = *new ColumnDefinition();
	col->SetWidth(gl);
	columnCollection->Add(col);
}

Label* LobbyGUI::AddLabelWithStyle(const LambdaEngine::String& name, Panel* pParent, const LambdaEngine::String& styleKey, const LambdaEngine::String& content)
{
	Ptr<Label> label = *new Label();

	if (name != "")
	{
		label->SetName(name.c_str());
		RegisterName(name, label);
	}

	if (content != "")
		label->SetContent(content.c_str());

	Style* pStyle = FrameworkElement::FindResource<Style>(styleKey.c_str());
	label->SetStyle(pStyle);
	pParent->GetChildren()->Add(label);

	return label;
}

void LobbyGUI::AddTextBoxWithColor(const LambdaEngine::String& name, Noesis::Panel* pParent, const LambdaEngine::String& styleKey, const LambdaEngine::String& text, const glm::vec3& color)
{
	Ptr<TextBlock> textBlock = *new TextBlock();

	if (name != "")
	{
		textBlock->SetName(name.c_str());
		RegisterName(name, textBlock);
	}

	textBlock->SetText(text.c_str());

	Ptr<SolidColorBrush> pBrush = *new SolidColorBrush();
	pBrush->SetColor(Color(color.r, color.g, color.b));
	textBlock->SetBackground(pBrush);

	Style* pStyle = FrameworkElement::FindResource<Style>(styleKey.c_str());
	textBlock->SetStyle(pStyle);
	pParent->GetChildren()->Add(textBlock);
}

void LobbyGUI::RegisterName(const LambdaEngine::String& name, BaseComponent* comp)
{
	FrameworkElement::GetView()->GetContent()->RegisterName(name.c_str(), comp);
}

void LobbyGUI::UnregisterName(const LambdaEngine::String& name)
{
	FrameworkElement::GetView()->GetContent()->UnregisterName(name.c_str());
}

void LobbyGUI::CreateHostIcon(Panel* parent)
{
	Ptr<Viewbox> viewBox	= *new Viewbox();
	Ptr<Path> path			= *new Path();

	Style* pStyle = FrameworkElement::FindResource<Style>("CrownPath");
	path->SetStyle(pStyle);
	viewBox->SetChild(path);

	// viewBox->SetNodeParent(parent);
	parent->GetChildren()->Add(viewBox);

	RegisterName("host_icon", viewBox);
}

Grid* LobbyGUI::GetPlayerGrid(const Player& player)
{
	const LambdaEngine::String& uid = std::to_string(player.GetUID());

	Grid* pGrid = m_pTeam1StackPanel->FindName<Grid>((uid + "_grid").c_str());
	if (pGrid)
	{
		return pGrid;
	}

	pGrid = m_pTeam2StackPanel->FindName<Grid>((uid + "_grid").c_str());
	return pGrid;
}
