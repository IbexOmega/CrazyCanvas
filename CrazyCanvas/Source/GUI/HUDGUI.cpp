#include "Game/State.h"

#include "Engine/EngineConfig.h"

#include "GUI/HUDGUI.h"
#include "GUI/Core/GUIApplication.h"


#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

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

	UpdateScore();
}


bool HUDGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonGrowClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonScoreClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonShootClick);
	return false;
}

bool HUDGUI::ApplyDamage(float damage)
{
	//Returns false if player is dead

	m_GUIState.DamageTaken += damage;
	std::string life;

	if (m_GUIState.DamageTaken < m_GUIState.LifeMaxHeight)
	{
		float percent = (damage / m_GUIState.LifeMaxHeight) * 100;
		m_GUIState.Health -= percent;

		life = std::to_string((int)m_GUIState.Health) + " %";

		FrameworkElement::FindName<Rectangle>("HEALTH_RECT")->SetHeight(m_GUIState.DamageTaken);
		FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());
	}
	else
	{
		life = "0 %";
		FrameworkElement::FindName<TextBlock>("HEALTH_DISPLAY")->SetText(life.c_str());

		{//Resets
			m_GUIState.DamageTaken = 0.0;
			m_GUIState.Health = 100.0;
			FrameworkElement::FindName<Rectangle>("HEALTH_RECT")->SetHeight(m_GUIState.DamageTaken);
		}
		return false;
	}
	return true;
}

bool HUDGUI::UpdateScore()
{
	//Returns false if game Over
	std::string scoreString;

	m_GUIState.CurrentScore++;

	scoreString = std::to_string(m_GUIState.CurrentScore) + "/" + std::to_string(MAX_SCORE);

	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY")->SetText(scoreString.c_str());

	if (m_GUIState.CurrentScore == MAX_SCORE)
	{
		m_GUIState.CurrentScore = 0;
		return false;
	}
	return true;
}

bool HUDGUI::UpdateAmmo(const int32 currentAmmo, const int32 ammoCap)
{
	//Returns false if Out Of Ammo
	std::string ammoString;

	m_GUIState.Ammo			= currentAmmo;
	m_GUIState.AmmoCapacity = ammoCap;
	
	ammoString = std::to_string(m_GUIState.Ammo) + "/" + std::to_string(m_GUIState.AmmoCapacity);

	FrameworkElement::FindName<TextBlock>("AMMUNITION_DISPLAY")->SetText(ammoString.c_str());

	if (currentAmmo <= 0)
	{
		return false;
	}
	return true;
}

void HUDGUI::InitGUI()
{
	Rectangle* pHpRect = FrameworkElement::FindName<Rectangle>("HEALTH_RECT");

	m_GUIState.DamageTaken		= 0.0;
	m_GUIState.LifeMaxHeight	= pHpRect->GetHeight();
	m_GUIState.Health			= 100.0;
	m_GUIState.Ammo				= m_GUIState.AmmoCapacity;
	m_GUIState.CurrentScore		= 0;

	pHpRect->SetHeight(0.0);

	std::string scoreString;
	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);
	scoreString = std::to_string(m_GUIState.CurrentScore) + "/" + std::to_string(MAX_SCORE);

	FrameworkElement::FindName<TextBlock>("AMMUNITION_DISPLAY")->SetText(ammoString.c_str());
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY")->SetText(scoreString.c_str());
}
