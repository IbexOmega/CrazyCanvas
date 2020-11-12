#include "GUI/GameOverGUI.h"

#include "Game/StateManager.h"
#include "States/MultiplayerState.h"

#include "Application/API/CommonApplication.h"

#include "World/Player/PlayerActionSystem.h"

#include "Resources/ResourceManager.h"

#include "NoesisPCH.h"


GameOverGUI::GameOverGUI()
{
	m_GameOverSound = LambdaEngine::ResourceManager::LoadSoundEffect2DFromFile("2currency.wav");
}

GameOverGUI::~GameOverGUI()
{
}

bool GameOverGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnReturnToLobbyButtonClick);

	return false;
}

void GameOverGUI::OnReturnToLobbyButtonClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	LambdaEngine::State* pLobbyState = DBG_NEW MultiplayerState();
	LambdaEngine::StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, LambdaEngine::STATE_TRANSITION::POP_AND_PUSH);
}

void GameOverGUI::DisplayGameOverGrid(bool isVisible)
{
	//This is
	if(isVisible)
		m_pGameOverGrid->SetVisibility(Noesis::Visibility_Visible);
	else
		m_pGameOverGrid->SetVisibility(Noesis::Visibility_Hidden);

	m_pShowWindowStoryboard->Begin();
	LambdaEngine::ResourceManager::GetSoundEffect2D(m_GameOverSound)->PlayOnce(1.0f);
}

void GameOverGUI::SetMostKillsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostKillsText->SetText(playerName.c_str());
	FindName<Noesis::TextBlock>("MOST_KILLS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetMostDeathsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostDeathsText->SetText(playerName.c_str());
	FindName<Noesis::TextBlock>("MOST_DEATHS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetMostFlagsStats(uint8 score, const LambdaEngine::String& playerName)
{
	m_pMostFlagsText->SetText(playerName.c_str());
	FindName<Noesis::TextBlock>("MOST_FLAGS_SCORE")->SetText(std::to_string(score).c_str());
}

void GameOverGUI::SetWinningTeam(uint8 winningTeamIndex)
{
	if (winningTeamIndex == 0)
		m_pWinningTeamText->SetText("Team Blue won this round");
	else
		m_pWinningTeamText->SetText("Team Red won this round");
}

void GameOverGUI::InitGUI()
{
	Noesis::GUI::LoadComponent(this, "GameOverGUI.xaml");

	m_pGameOverGrid = FindName<Noesis::Grid>("GAME_OVER_GRID");

	m_pShowWindowStoryboard = FindResource<Noesis::Storyboard>("ShowGameOverWindow");

	m_pWinningTeamText = FindName<Noesis::TextBlock>("WINNING_TEAM_TEXT");

	m_pMostKillsText	= FindName<Noesis::TextBlock>("MOST_KILLS_NAME");
	m_pMostDeathsText	= FindName<Noesis::TextBlock>("MOST_DEATHS_NAME");
	m_pMostFlagsText	= FindName<Noesis::TextBlock>("MOST_FLAGS_NAME");

	LambdaEngine::CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
}
