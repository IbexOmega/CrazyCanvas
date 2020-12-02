#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "World/Player/PlayerActionSystem.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/TabItem.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/StackPanel.h"
#include "NsGui/ObservableCollection.h"
#include "NsGui/Button.h"

#include "Lobby/PlayerManagerBase.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "Lobby/Player.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"


enum class EPlayerProperty
{
	PLAYER_PROPERTY_NAME,
	PLAYER_PROPERTY_KILLS,
	PLAYER_PROPERTY_DEATHS,
	PLAYER_PROPERTY_FLAGS_CAPTURED,
	PLAYER_PROPERTY_FLAGS_DEFENDED,
	PLAYER_PROPERTY_PING,
};

class ScoreBoardGUI : public Noesis::UserControl
{
public:
	ScoreBoardGUI();
	~ScoreBoardGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void InitGUI();

	void DisplayScoreboardMenu(bool visible);

	void AddPlayer(const Player& newPlayer);
	void RemovePlayer(const Player& player);
	void UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value);
	void UpdateAllPlayerProperties(const Player& player);
	void UpdatePlayerAliveStatus(uint64 UID, bool isAlive);

private:

	// Helpers
	void AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column);

	NS_IMPLEMENT_INLINE_REFLECTION_(ScoreBoardGUI, Noesis::UserControl, "CrazyCanvas.ScoreBoardGUI")

private:

	Noesis::Grid* m_pScoreboardGrid = nullptr;

	Noesis::StackPanel* m_pTeam1StackPanel = nullptr;
	Noesis::StackPanel* m_pTeam2StackPanel = nullptr;

	LambdaEngine::THashTable<uint64, Noesis::Grid*> m_PlayerGrids;

	bool m_ScoreboardVisible = false;
};