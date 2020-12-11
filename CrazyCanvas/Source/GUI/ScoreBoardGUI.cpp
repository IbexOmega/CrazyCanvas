#include "GUI/ScoreBoardGUI.h"

#include "GUI/GUIHelpers.h"
#include "Lobby/PlayerManagerClient.h"

#include "Teams/TeamHelper.h"

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

	m_pTeam1StackPanel = FrameworkElement::FindName<Noesis::StackPanel>("TEAM_1_STACK_PANEL");
	m_pTeam2StackPanel = FrameworkElement::FindName<Noesis::StackPanel>("TEAM_2_STACK_PANEL");

	Noesis::Ptr<Noesis::SolidColorBrush> pteamBrush1 = *new Noesis::SolidColorBrush();
	Noesis::Ptr<Noesis::SolidColorBrush> pteamBrush2 = *new Noesis::SolidColorBrush();

	glm::vec3 teamColor1 = TeamHelper::GetTeamColor(1);
	glm::vec3 teamColor2 = TeamHelper::GetTeamColor(2);

	Noesis::Color Color1(teamColor1.r, teamColor1.g, teamColor1.b);
	Noesis::Color Color2(teamColor2.r, teamColor2.g, teamColor2.b);

	pteamBrush1->SetColor(Color1);
	pteamBrush2->SetColor(Color2);

	FrameworkElement::FindName<Noesis::Label>("TEAM_1_LABEL")->SetForeground(pteamBrush1);
	FrameworkElement::FindName<Noesis::Label>("TEAM_2_LABEL")->SetForeground(pteamBrush2);
}

void ScoreBoardGUI::UpdatePlayerAliveStatus(const Player& player)
{
	if (player.IsSpectator())
		return;

	auto iterator = m_PlayerGrids.find(player.GetUID());

	if (iterator == m_PlayerGrids.end())
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", player.GetUID());
		return;
	}
	Grid* pGrid = iterator->second;

	Label* nameLabel = static_cast<Label*>(pGrid->GetChildren()->Get(0));

	LOG_ERROR("[HUDGUI]: Name: %s", nameLabel->GetContent()->ToString().Str());

	SolidColorBrush* pBrush = static_cast<SolidColorBrush*>(nameLabel->GetForeground());
	if (player.IsDead())
	{
		pBrush->SetColor(Color::LightGray());
		nameLabel->SetForeground(pBrush);
	}
	else
	{
		pBrush->SetColor(Color::White());
		nameLabel->SetForeground(pBrush);
	}
}

void ScoreBoardGUI::UpdateAllPlayerProperties(const Player& player)
{
	if (player.IsSpectator())
		return;

	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_NAME, player.GetName());
	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_KILLS, std::to_string(player.GetKills()));
	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_DEATHS, std::to_string(player.GetDeaths()));
	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED, std::to_string(player.GetFlagsCaptured()));
	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED, std::to_string(player.GetFlagsDefended()));
	UpdatePlayerProperty(player, EPlayerProperty::PLAYER_PROPERTY_PING, std::to_string(player.GetPing()));
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

void ScoreBoardGUI::UpdatePlayerProperty(const Player& player, EPlayerProperty property, const LambdaEngine::String& value)
{
	if (player.IsSpectator())
		return;

	uint8 index = 0;
	switch (property)
	{
	case EPlayerProperty::PLAYER_PROPERTY_NAME:				index = 0; break;
	case EPlayerProperty::PLAYER_PROPERTY_KILLS:			index = 1; break;
	case EPlayerProperty::PLAYER_PROPERTY_DEATHS:			index = 2; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED:	index = 3; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED:	index = 4; break;
	case EPlayerProperty::PLAYER_PROPERTY_PING:				index = 5; break;
	default: LOG_WARNING("Enum not supported"); return;
	}

	auto iterator = m_PlayerGrids.find(player.GetUID());

	if (iterator == m_PlayerGrids.end())
	{
		LOG_WARNING("Player with UID: &lu not found!", player.GetUID());
		return;
	}
	Grid* pGrid = iterator->second;

	Label* pLabel = static_cast<Label*>(pGrid->GetChildren()->Get(index));
	pLabel->SetContent(value.c_str());
}

void ScoreBoardGUI::RemovePlayer(const Player& player)
{
	if (player.IsSpectator())
		return;

	auto iterator = m_PlayerGrids.find(player.GetUID());

	if (iterator == m_PlayerGrids.end())
	{
		LOG_WARNING("Tried to delete \"%s\", but could not find player UID.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}
	Grid* pGrid = iterator->second;

	if (!pGrid)
	{
		LOG_WARNING("Tried to delete \"%s\", but could not find grid.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}

	m_PlayerGrids.erase(player.GetUID());

	if (player.GetTeam() == 1)
	{
		m_pTeam1StackPanel->GetChildren()->Remove(pGrid);
	}
	else if (player.GetTeam() == 2)
	{
		m_pTeam2StackPanel->GetChildren()->Remove(pGrid);
	}
	else
	{
		LOG_WARNING("Tried to remove player with unknown team. Playername: %s, team: %d", player.GetName().c_str(), player.GetTeam());
	}

}

void ScoreBoardGUI::AddPlayer(const Player& player)
{
	if (player.IsSpectator())
		return;

	Ptr<Grid> pGrid = *new Grid();
	m_PlayerGrids[player.GetUID()] = pGrid;

	pGrid->SetName(std::to_string(player.GetUID()).c_str());
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

	pNameLabel->SetContent(player.GetName().c_str());
	pNameLabel->SetForeground(pWhiteBrush);
	pNameLabel->SetFontSize(28.f);
	pNameLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	uint8 column = 1;
	pGrid->GetChildren()->Add(pNameLabel);
	pGrid->SetColumn(pNameLabel, column++);

	AddStatsLabel(pGrid, std::to_string(player.GetKills()), column++);
	AddStatsLabel(pGrid, std::to_string(player.GetDeaths()), column++);
	AddStatsLabel(pGrid, std::to_string(player.GetFlagsCaptured()), column++);
	AddStatsLabel(pGrid, std::to_string(player.GetFlagsDefended()), column++);
	AddStatsLabel(pGrid, std::to_string(player.GetPing()), column++);

	if (player.GetTeam() == 1)
	{
		m_pTeam1StackPanel->GetChildren()->Add(pGrid);
	}
	else if (player.GetTeam() == 2)
	{
		m_pTeam2StackPanel->GetChildren()->Add(pGrid);
	}
	else
	{
		LOG_WARNING("Unknown team on player \"%s\".\n\tUID: %lu\n\tTeam: %d",
			player.GetName().c_str(), player.GetUID(), player.GetTeam());
	}

	if (PlayerManagerClient::GetPlayerLocal()->GetUID() == player.GetUID())
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