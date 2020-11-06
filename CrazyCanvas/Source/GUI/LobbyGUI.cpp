#include "GUI/LobbyGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

using namespace Noesis;
using namespace LambdaEngine;

LobbyGUI::LobbyGUI()
{
	Noesis::GUI::LoadComponent(this, "Lobby.xaml");

	// Get commonly used elements
	m_pBlueTeamStackPanel	= FrameworkElement::FindName<StackPanel>("BlueTeamStackPanel");
	m_pRedTeamStackPanel	= FrameworkElement::FindName<StackPanel>("RedTeamStackPanel");
	m_pChatPanel			= FrameworkElement::FindName<StackPanel>("ChatStackPanel");
	m_pChatInputTextBox		= FrameworkElement::FindName<TextBox>("ChatInputTextBox");

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
		AddPlayer("testgubbe", 0);
	}
	else if (event.Key == EKey::KEY_O)
	{
		UpdatePlayerReady("testgubbe", true);
	}
	else if (event.Key == EKey::KEY_L)
	{
		UpdatePlayerReady("testgubbe", false);
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

void LobbyGUI::UpdateLobbySettings(LobbyGUI::LobbySettings newLobbySettings)
{
}

void LobbyGUI::SetLocalPlayerName(const LambdaEngine::String& playerName)
{
	m_LocalPlayerName = playerName;
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
