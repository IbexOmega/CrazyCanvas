#include "GUI/GameOverGUI.h"

#include "Game/StateManager.h"
#include "States/MultiplayerState.h"

#include "Application/API/CommonApplication.h"

#include "NoesisPCH.h"


GameOverGUI::GameOverGUI()
{

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
}

void GameOverGUI::SetWinningTeam(uint8 winningTeamIndex)
{
	LambdaEngine::String winningTeamString = "";
	if (winningTeamIndex == 0)
		winningTeamString = "Team Blue won this round";
	else
		winningTeamString = "Team Red won this round";

	m_pWinningTeamText->SetText(winningTeamString.c_str());
}

void GameOverGUI::InitGUI()
{
	Noesis::GUI::LoadComponent(this, "GameOverGUI.xaml");

	m_pGameOverGrid = FindName<Noesis::Grid>("GAME_OVER_GRID");

	m_pWinningTeamText = FindName<Noesis::TextBlock>("WINNING_TEAM_TEXT");

	m_pMostKillsText	= FindName<Noesis::TextBlock>("MOST_KILLS_TEXT");
	m_pMostDeathsText	= FindName<Noesis::TextBlock>("MOST_DEATHS_TEXT");
	m_pMostFlagsText	= FindName<Noesis::TextBlock>("MOST_FLAGS_TEXT");

	LambdaEngine::CommonApplication::Get()->SetMouseVisibility(true);
}
