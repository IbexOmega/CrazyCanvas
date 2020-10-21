#pragma once

#include "Containers/String.h"

#include "LambdaEngine.h"

#include "Application/API/Events/NetworkEvents.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/ObservableCollection.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#define MAX_SCORE 5
#define MAX_AMMO 100.0

struct GameGUIState
{
	float LifeMaxHeight;
	float DamageTaken;
	float Health;
	float Ammo;

	int8 CurrentScore;
};

class HUDGUI : public Noesis::Grid
{

public:
	HUDGUI(const LambdaEngine::String& xamlFile);
	~HUDGUI();

	void OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonShootClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonScoreClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	bool ApplyDamage(float damage);
	bool UpdateScore();
	bool UpdateAmmo();

private:

	void InitGUI();

	NS_IMPLEMENT_INLINE_REFLECTION_(HUDGUI, Noesis::Grid)

private:
	GameGUIState m_GUIState;
};