#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Grid.h>
#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/TextBlock.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

class GameOverGUI : public Noesis::UserControl
{
public:
	GameOverGUI();
	~GameOverGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnReturnToLobbyButtonClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void DisplayGameOverGrid(bool isVisible);
	void SetWinningTeam(uint8 winningTeamIndex);
	void InitGUI();

private:
	Noesis::Grid* m_pGameOverGrid = nullptr;

	Noesis::Storyboard* m_pShowWindowStoryboard = nullptr;

	Noesis::TextBlock* m_pWinningTeamText = nullptr;

	Noesis::TextBlock* m_pMostKillsText		= nullptr;
	Noesis::TextBlock* m_pMostDeathsText	= nullptr;
	Noesis::TextBlock* m_pMostFlagsText		= nullptr;

	GUID_Lambda m_GameOverSound;

	NS_IMPLEMENT_INLINE_REFLECTION_(GameOverGUI, Noesis::UserControl, "CrazyCanvas.GameOverGUI")

};
