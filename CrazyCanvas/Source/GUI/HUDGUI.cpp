#include "Game/State.h"

#include "Engine/EngineConfig.h"

#include "GUI/HUDGUI.h"
#include "GUI/Core/GUIApplication.h"


#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"

#include <string>


using namespace LambdaEngine;
using namespace Noesis;

HUDGUI::HUDGUI(const LambdaEngine::String& xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());

	Rectangle* pHpRect = FrameworkElement::FindName<Rectangle>("HEALTH_RECT");


	m_Damage		= 0.0;
	m_LifeMaxHeight = pHpRect->GetHeight();
	m_Health		= 100.0;


	pHpRect->SetHeight(0.0);
	//hpRect->SetMaxHeight((float)m_LifeMaxHeight);
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
	float percent = (damage / m_LifeMaxHeight) * 100;

	m_Damage += damage;
	m_Health -= percent;

	FrameworkElement::FindName<Rectangle>("HEALTH_RECT")->SetHeight(m_Damage);

	std::string life = std::to_string((int)m_Health) + " %";

	if (m_Damage > m_LifeMaxHeight)
	{
		life = "0 %";
		FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());
		return false;
	}
	FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());

	return true;
}
