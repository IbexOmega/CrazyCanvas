#include "Game/State.h"

#include "Engine/EngineConfig.h"

#include "GUI/HUDGUI.h"
#include "GUI/Core/GUIApplication.h"


#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"

#include <string>


using namespace LambdaEngine;
using namespace Noesis;

HUDGUI::HUDGUI(const LambdaEngine::String& xamlFile) : 
	m_GUIState()
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	InitGUI();
}


HUDGUI::~HUDGUI()
{
	//EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &HUDGUI::OnLANServerFound);
}

void HUDGUI::OnButtonGrowClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ApplyDamage(10);
}

void HUDGUI::OnButtonShootClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

}

void HUDGUI::OnButtonScoreClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

}




bool HUDGUI::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonGrowClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonScoreClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonShootClick);
	return false;
}

bool HUDGUI::ApplyDamage(float damage)
{
	//Returns false if player is dead
	float percent = (damage / m_GUIState.LifeMaxHeight) * 100;

	m_GUIState.Damage += damage;
	m_GUIState.Health -= percent;

	FrameworkElement::FindName<Rectangle>("HEALTH_RECT")->SetHeight(m_GUIState.Damage);

	std::string life = std::to_string((int)m_GUIState.Health) + " %";

	if (m_GUIState.Damage > m_GUIState.LifeMaxHeight)
	{
		life = "0 %";
		FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());
		return false;
	}
	FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());

	return true;
}

void HUDGUI::InitGUI()
{
	Rectangle* pHpRect = FrameworkElement::FindName<Rectangle>("HEALTH_RECT");

	m_GUIState.Damage = 0.0;
	m_GUIState.LifeMaxHeight = pHpRect->GetHeight();
	m_GUIState.Health = 100.0;
	m_GUIState.Ammo = 0.0;
	m_GUIState.CurrentScore = 0;

	pHpRect->SetHeight(0.0);

	std::string scoreString;
	std::string ammoString;

	ammoString = std::to_string(m_GUIState.Ammo) + "/100";
	scoreString = std::to_string(m_GUIState.CurrentScore) + "/5";

	FrameworkElement::FindName<TextBlock>("AMMUNITION_DISPLAY")->SetText(ammoString.c_str());
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY")->SetText(ammoString.c_str());
}
