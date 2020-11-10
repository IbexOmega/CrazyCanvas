#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Player/ProjectileComponent.h"

#include "World/Player/PlayerActionSystem.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/Border.h"
#include "NsGui/ObservableCollection.h"
#include "NsGui/Button.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

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

class HUDGUI : public Noesis::Grid
{
public:
	HUDGUI();
	~HUDGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonShootClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonScoreClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);
	bool OpenEscapeMenu(const LambdaEngine::KeyPressedEvent& event);

	// Escape GUI
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// Settings
	void OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonChangeKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// Key bindings
	void OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplyKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void UpdateCountdown(uint8 countDownTime);

	void DisplayHitIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal);

private:

	void InitGUI();

	void SetDefaultSettings();
	void SetDefaultKeyBindings();
	void SetRenderStagesInactive();
	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;

	Noesis::Border* m_pWaterAmmoRect;
	Noesis::Border* m_pPaintAmmoRect;

	Noesis::TextBlock* m_pWaterAmmoText;
	Noesis::TextBlock* m_pPaintAmmoText;

	// EscapeGUI
	bool 			m_ListenToCallbacks		= false;
	Noesis::Button* m_pSetKeyButton			= nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;

	bool			m_RayTracingEnabled		= false;
	bool			m_MeshShadersEnabled	= false;
	bool			m_EscapeMenuEnabled		= false;

	bool			m_MouseEnabled			= false;

	Noesis::Grid*	m_pEscapeGrid			= nullptr;
	Noesis::Grid*	m_pSettingsGrid			= nullptr;
	Noesis::Grid*	m_pKeyBindingsGrid		= nullptr;

	LambdaEngine::TStack<Noesis::FrameworkElement*> m_ContextStack;
	Noesis::Grid* m_pHitIndicatorGrid;
};