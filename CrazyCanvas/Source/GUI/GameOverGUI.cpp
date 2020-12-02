#include "GUI/GameOverGUI.h"

#include "Game/StateManager.h"
#include "States/LobbyState.h"
#include "States/PlaySessionState.h"

#include "Application/API/CommonApplication.h"

#include "World/Player/PlayerActionSystem.h"

#include "Teams/TeamHelper.h"

#include "Resources/ResourceManager.h"

#include "Lobby/PlayerManagerClient.h"

#include "NoesisPCH.h"

using namespace Noesis;
using namespace LambdaEngine;

GameOverGUI::GameOverGUI()
{
	//m_GameOverSound = ResourceManager::LoadSoundEffect2DFromFile("2currency.wav");
}

GameOverGUI::~GameOverGUI()
{
}

bool GameOverGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Button, Click, OnReturnToLobbyButtonClick);

	return false;
}

void GameOverGUI::OnReturnToLobbyButtonClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	const PacketGameSettings& gameSettings = PlaySessionState::GetInstance()->GetGameSettings();
	State* pLobbyState = DBG_NEW LobbyState(gameSettings, PlayerManagerClient::GetPlayerLocal());
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);
}

void GameOverGUI::DisplayGameOverGrid(bool isVisible)
{
	//This is
	if(isVisible)
		m_pGameOverGrid->SetVisibility(Visibility_Visible);
	else
		m_pGameOverGrid->SetVisibility(Visibility_Hidden);

	m_pShowWindowStoryboard->Begin();
	//ResourceManager::GetSoundEffect2D(m_GameOverSound)->PlayOnce(1.0f);
}

void GameOverGUI::SetMostKillsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostKillsText->SetText(playerName.c_str());
	FindName<TextBlock>("MOST_KILLS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetMostDeathsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostDeathsText->SetText(playerName.c_str());
	FindName<TextBlock>("MOST_DEATHS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetMostFlagsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostFlagsText->SetText(playerName.c_str());
	FindName<TextBlock>("MOST_FLAGS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetWinningTeam(uint8 winningTeamIndex)
{
	Noesis::Ptr<Noesis::SolidColorBrush> pBrush = *new Noesis::SolidColorBrush();

	glm::vec3 teamColor = TeamHelper::GetTeamColor(winningTeamIndex);
	Noesis::Color winningTeamColor(teamColor.r, teamColor.g, teamColor.b);
	
	pBrush->SetColor(winningTeamColor);

	m_pWinningTeamText->SetForeground(pBrush);

	LambdaEngine::String gameOvertext = winningTeamIndex == 1 ? "Team 1 won this round" : "Team 2 won this round";

	m_pWinningTeamText->SetText(gameOvertext.c_str());

}

void GameOverGUI::InitGUI()
{
	Noesis::GUI::LoadComponent(this, "GameOverGUI.xaml");

	m_pGameOverGrid = FindName<Grid>("GAME_OVER_GRID");

	m_pShowWindowStoryboard = FindResource<Storyboard>("ShowGameOverWindow");

	m_pWinningTeamText = FindName<TextBlock>("WINNING_TEAM_TEXT");

	m_pMostKillsText	= FindName<TextBlock>("MOST_KILLS_NAME");
	m_pMostDeathsText	= FindName<TextBlock>("MOST_DEATHS_NAME");
	m_pMostFlagsText	= FindName<TextBlock>("MOST_FLAGS_NAME");

	CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
}
