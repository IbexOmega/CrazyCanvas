#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Player/ProjectileComponent.h"
#include "ECS/Components/GUI/ProjectedGUIComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/Image.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/StackPanel.h"
#include "NsGui/Rectangle.h"
#include "NsGui/ObservableCollection.h"
#include "NsGui/Button.h"

#include "Lobby/PlayerManagerBase.h"

#include "GUI/EscapeMenuGUI.h"
#include "GUI/KillFeedGUI.h"
#include "GUI/ScoreBoardGUI.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "Lobby/Player.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#define MAX_AMMO 100

struct GameGUIState
{
	int32 Health;
	int32 MaxHealth = 100;

	LambdaEngine::TArray<uint32> Scores;

	int32 Ammo;
	int32 AmmoCapacity;
};

typedef  std::pair<uint8, const Player*> PlayerPair;

class HUDGUI : public Noesis::Grid
{
public:
	HUDGUI();
	~HUDGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);

	void UpdateCountdown(uint8 countDownTime);

	void DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal);
	void DisplayHitIndicator();
	void DisplayGameOverGrid(uint8 winningTeamIndex, PlayerPair& mostKills, PlayerPair& mostDeaths, PlayerPair& mostFlags);
	void DisplayPrompt(const LambdaEngine::String& promptMessage);

	void UpdateKillFeed(const LambdaEngine::String& killed, const LambdaEngine::String& killer, uint8 killedPlayerTeamIndex);
	void UpdateKillFeedTimer(LambdaEngine::Timestamp delta);

	void ProjectGUIIndicator(const glm::mat4& viewProj, const glm::vec3& worldPos, LambdaEngine::Entity entity);
	void CreateProjectedGUIElement(LambdaEngine::Entity entity, uint8 localTeamIndex, uint8 teamIndex = UINT8_MAX);
	void RemoveProjectedGUIElement(LambdaEngine::Entity entity);

	void SetWindowSize(uint32 width, uint32 height);

	ScoreBoardGUI* GetScoreBoard() const;

private:
	void InitGUI();

	void TranslateIndicator(Noesis::Transform* pTranslation, LambdaEngine::Entity entity);
	void SetIndicatorOpacity(float32 value, LambdaEngine::Entity entity);
	void SetRenderStagesInactive();

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;
	bool m_IsGameOver = false;

	Noesis::Image* m_pWaterAmmoRect				= nullptr;
	Noesis::Image* m_pPaintAmmoRect				= nullptr;
	Noesis::Image* m_pHealthRect				= nullptr;

	Noesis::TextBlock* m_pWaterAmmoText			= nullptr;
	Noesis::TextBlock* m_pPaintAmmoText			= nullptr;

	Noesis::Grid* m_pHUDGrid					= nullptr;

	Noesis::Grid* m_pHitIndicatorGrid			= nullptr;
	Noesis::Grid* m_pScoreboardGrid				= nullptr;

	Noesis::Grid* m_pRedScoreGrid				= nullptr;
	Noesis::Grid* m_pBlueScoreGrid				= nullptr;


	glm::vec2 m_WindowSize = glm::vec2(1.0f);

	LambdaEngine::THashTable<uint64, Noesis::Grid*> m_PlayerGrids;

	std::unordered_map<LambdaEngine::Entity, Noesis::Rectangle*> m_ProjectedElements;

	KillFeedGUI* m_pKillFeedGUI		= nullptr;
	EscapeMenuGUI* m_pEscMenuGUI	= nullptr;
	ScoreBoardGUI* m_pScoreBoardGUI = nullptr;
};