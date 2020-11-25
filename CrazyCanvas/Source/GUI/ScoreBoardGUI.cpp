#include "GUI/ScoreBoardGUI.h"

#include "GUI/GUIHelpers.h"
#include "Lobby/PlayerManagerClient.h"

#include "NoesisPCH.h"

using namespace LambdaEngine;
using namespace Noesis;

ScoreBoardGUI::ScoreBoardGUI()
{
	Noesis::GUI::LoadComponent(this, "ScoreBoardGUI.xaml");

	InitGUI();
}

ScoreBoardGUI::~ScoreBoardGUI()
{
	m_PlayerGrids.clear();
}

bool ScoreBoardGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	UNREFERENCED_VARIABLE(pSource);
	UNREFERENCED_VARIABLE(pEvent);
	UNREFERENCED_VARIABLE(pHandler);
	return false;
}

void ScoreBoardGUI::InitGUI()
{
	m_pScoreboardGrid = FrameworkElement::FindName<Noesis::Grid>("SCOREBOARD_GRID");

	m_pBlueTeamStackPanel = FrameworkElement::FindName<Noesis::StackPanel>("BLUE_TEAM_STACK_PANEL");
	m_pRedTeamStackPanel = FrameworkElement::FindName<Noesis::StackPanel>("RED_TEAM_STACK_PANEL");
}


void ScoreBoardGUI::UpdatePlayerAliveStatus(uint64 UID, bool isAlive)
{
	if (!m_PlayerGrids.contains(UID))
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", UID);
		return;
	}
	Grid* pGrid = m_PlayerGrids[UID];

	Label* nameLabel = static_cast<Label*>(pGrid->GetChildren()->Get(0));

	LOG_ERROR("[HUDGUI]: Name: %s", nameLabel->GetContent()->ToString().Str());

	SolidColorBrush* pBrush = static_cast<SolidColorBrush*>(nameLabel->GetForeground());
	if (isAlive)
	{
		pBrush->SetColor(Color::White());
		nameLabel->SetForeground(pBrush);
	}
	else
	{
		pBrush->SetColor(Color::LightGray());
		nameLabel->SetForeground(pBrush);
	}
}

void ScoreBoardGUI::UpdateAllPlayerProperties(const Player& player)
{
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_NAME, player.GetName());
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_KILLS, std::to_string(player.GetKills()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_DEATHS, std::to_string(player.GetDeaths()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED, std::to_string(player.GetFlagsCaptured()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED, std::to_string(player.GetFlagsDefended()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_PING, std::to_string(player.GetPing()));
}

void ScoreBoardGUI::AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column)
{
	Ptr<Label> pLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pLabel->SetContent(content.c_str());
	pLabel->SetForeground(pWhiteBrush);
	pLabel->SetHorizontalAlignment(HorizontalAlignment::HorizontalAlignment_Right);
	pLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	pParentGrid->GetChildren()->Add(pLabel);
	pParentGrid->SetColumn(pLabel, column);
}

void ScoreBoardGUI::UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value)
{
	uint8 index = 0;
	switch (property)
	{
	case EPlayerProperty::PLAYER_PROPERTY_NAME:				index = 0; break;
	case EPlayerProperty::PLAYER_PROPERTY_KILLS:			index = 1; break;
	case EPlayerProperty::PLAYER_PROPERTY_DEATHS:			index = 2; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED:	index = 3; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED:	index = 4; break;
	case EPlayerProperty::PLAYER_PROPERTY_PING:				index = 5; break;
	default: LOG_WARNING("[HUDGUI]: Enum not supported"); return;
	}

	if (!m_PlayerGrids.contains(playerUID))
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", playerUID);
		return;
	}
	Grid* pGrid = m_PlayerGrids[playerUID];

	Label* pLabel = static_cast<Label*>(pGrid->GetChildren()->Get(index));
	pLabel->SetContent(value.c_str());
}

void ScoreBoardGUI::RemovePlayer(const Player& player)
{
	if (!m_PlayerGrids.contains(player.GetUID()))
	{
		LOG_WARNING("[HUDGUI]: Tried to delete \"%s\", but could not find player UID.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}
	Grid* pGrid = m_PlayerGrids[player.GetUID()];

	if (!pGrid)
	{
		LOG_WARNING("[HUDGUI]: Tried to delete \"%s\", but could not find grid.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}

	m_PlayerGrids.erase(player.GetUID());

	if (player.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(pGrid);
	}
	else if (player.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(pGrid);
	}
	else
	{
		LOG_WARNING("[HUDGUI]: Tried to remove player with unknown team. Playername: %s, team: %d", player.GetName().c_str(), player.GetTeam());
	}

}

void ScoreBoardGUI::AddPlayer(const Player& newPlayer)
{
	Ptr<Grid> pGrid = *new Grid();
	m_PlayerGrids[newPlayer.GetUID()] = pGrid;

	pGrid->SetName(std::to_string(newPlayer.GetUID()).c_str());
	FrameworkElement::GetView()->GetContent()->RegisterName(pGrid->GetName(), pGrid);

	ColumnDefinitionCollection* pColumnCollection = pGrid->GetColumnDefinitions();
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 2.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);

	// Name is different than other labels
	Ptr<Label> pNameLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pNameLabel->SetContent(newPlayer.GetName().c_str());
	pNameLabel->SetForeground(pWhiteBrush);
	pNameLabel->SetFontSize(28.f);
	pNameLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	uint8 column = 1;
	pGrid->GetChildren()->Add(pNameLabel);
	pGrid->SetColumn(pNameLabel, column++);

	AddStatsLabel(pGrid, std::to_string(newPlayer.GetKills()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetDeaths()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsCaptured()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsDefended()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetPing()), column++);

	if (newPlayer.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else if (newPlayer.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else
	{
		LOG_WARNING("[HUDGUI]: Unknown team on player \"%s\".\n\tUID: %lu\n\tTeam: %d",
			newPlayer.GetName().c_str(), newPlayer.GetUID(), newPlayer.GetTeam());
	}

	if (PlayerManagerClient::GetPlayerLocal()->GetUID() == newPlayer.GetUID())
	{
		Ptr<Image> localPlayerIcon = *new Image();
		Ptr<BitmapImage> srcImage = *new BitmapImage();
		srcImage->SetUriSource(Noesis::Uri::Uri("splashes/splash_green.png"));
		localPlayerIcon->SetSource(srcImage);
		localPlayerIcon->SetWidth(40);
		localPlayerIcon->SetHeight(40);
		pGrid->GetChildren()->Add(localPlayerIcon);
		pGrid->SetColumn(localPlayerIcon, 0);
	}
}

void ScoreBoardGUI::DisplayScoreboardMenu(bool visible)
{
	// Toggle to visible
	if (!m_ScoreboardVisible && visible)
	{
		m_ScoreboardVisible = true;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Visible);
	}
	// Toggle to hidden
	else if (m_ScoreboardVisible && !visible)
	{
		m_ScoreboardVisible = false;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Hidden);
	}
}