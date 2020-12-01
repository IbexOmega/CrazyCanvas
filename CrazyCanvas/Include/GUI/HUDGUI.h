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
#include "NsGui/Ellipse.h"
#include "NsGui/ObservableCollection.h"
#include "NsGui/Button.h"

#include "Lobby/PlayerManagerBase.h"

#include "GUI/EscapeMenuGUI.h"
#include "GUI/KillFeedGUI.h"
#include "GUI/ScoreBoardGUI.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "Time/API/Timestamp.h"

#include "Lobby/Player.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#define MAX_AMMO 100

struct GameGUIState
{
	int32 Health;
	int32 MaxHealth = 100;

	LambdaEngine::TArray<uint32> Scores;

	int32 WaterAmmo = 50;
	int32 PaintAmmo = 50;

	int32 WaterAmmoCapacity = 50;
	int32 PaintAmmoCapacity = 50;
};

typedef std::pair<int16, const Player*> PlayerPair;

class HUDGUI : public Noesis::Grid
{
public:
	HUDGUI();
	~HUDGUI();

	void FixedTick(LambdaEngine::Timestamp delta);

	void AnimateReload(const float32 timePassed);

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);

	void Reload(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, bool isReloading);
	void AbortReload(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo);

	void UpdateCountdown(uint8 countDownTime);

	void DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal);
	void DisplayHitIndicator();
	void DisplayGameOverGrid(uint8 winningTeamIndex, PlayerPair& mostKills, PlayerPair& mostDeaths, PlayerPair& mostFlags);
	void DisplayPrompt(const LambdaEngine::String& promptMessage, bool isSmallPrompt, const uint8 teamIndex);
	void DisplaySpecateText(const LambdaEngine::String& name, bool isSpectating);
	void CancelSmallPrompt();

	void UpdateKillFeed(const LambdaEngine::String& killed, const LambdaEngine::String& killer, uint8 killedPlayerTeamIndex);
	void UpdateKillFeedTimer(LambdaEngine::Timestamp delta);

	void ProjectGUIIndicator(const glm::mat4& viewProj, const glm::vec3& worldPos, LambdaEngine::Entity entity);
	void CreateProjectedGUIElement(LambdaEngine::Entity entity, uint8 localTeamIndex, uint8 teamIndex = UINT8_MAX);
	void RemoveProjectedGUIElement(LambdaEngine::Entity entity);

	void SetWindowSize(uint32 width, uint32 height);
	void ShowHUD(const bool isVisible);

	ScoreBoardGUI* GetScoreBoard() const;

private:
	void InitGUI();

	void InitScore();

	void TranslateIndicator(Noesis::Transform* pTranslation, LambdaEngine::Entity entity);
	void SetIndicatorOpacity(float32 value, LambdaEngine::Entity entity);
	void SetRenderStagesInactive();

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;

	bool m_IsGameOver	= false;
	bool m_IsReloading = false;

	float m_WaterAmmoScale = 0.0f;
	float m_PaintAmmoScale = 0.0f;

	float m_WaterAmmoFactor = 0.0f;
	float m_PaintAmmoFactor = 0.0f;

	float32 m_ReloadAnimationTime = 2.0f;

	Noesis::Image* m_pWaterAmmoRect				= nullptr;
	Noesis::Image* m_pPaintAmmoRect				= nullptr;
	Noesis::Image* m_pPaintDropRect				= nullptr;
	Noesis::Image* m_pHealthRect				= nullptr;

	Noesis::TextBlock* m_pWaterAmmoText			= nullptr;
	Noesis::TextBlock* m_pPaintAmmoText			= nullptr;

	Noesis::TextBlock* m_pSpectatePlayerText	= nullptr;

	Noesis::Grid* m_pHUDGrid					= nullptr;

	Noesis::Grid* m_pHitIndicatorGrid			= nullptr;
	Noesis::Grid* m_pScoreboardGrid				= nullptr;

	Noesis::TextBlock* m_pTeam1Score = nullptr;
	Noesis::TextBlock* m_pTeam2Score = nullptr;

	glm::vec2 m_WindowSize = glm::vec2(1.0f);

	std::unordered_map<LambdaEngine::Entity, Noesis::Grid*> m_ProjectedElements;

	KillFeedGUI* m_pKillFeedGUI		= nullptr;
	EscapeMenuGUI* m_pEscMenuGUI	= nullptr;
	ScoreBoardGUI* m_pScoreBoardGUI = nullptr;
};