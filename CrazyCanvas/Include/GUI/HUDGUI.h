#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "ECS/Components/Player/ProjectileComponent.h"

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

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

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
	HUDGUI(const LambdaEngine::String& xamlFile);
	~HUDGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonShootClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonScoreClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool UpdateHealth(int32 currentHealth);
	bool UpdateScore();
	bool UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType);

private:

	void InitGUI();

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;

	Noesis::Border* m_pWaterAmmoRect;
	Noesis::Border* m_pPaintAmmoRect;
};