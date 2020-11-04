#include "Game/State.h"

#include "Engine/EngineConfig.h"

#include "GUI/HUDGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "Multiplayer/Packet/PacketType.h"

#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Match/Match.h"

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

	//ApplyDamage(10);
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

bool HUDGUI::UpdateHealth(int32 currentHealth)
{
	//Returns false if player is dead
	if (currentHealth != m_GUIState.Health)
	{
		Noesis::Border* pHpRect = FrameworkElement::FindName<Noesis::Border>("HEALTH_RECT");
		SolidColorBrush* pBrush = (SolidColorBrush*)pHpRect->GetBackground();
		Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();

		float healthScale = (float)currentHealth / (float)m_GUIState.MaxHealth;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(healthScale);
		pHpRect->SetRenderTransform(scale);

		std::string hpString = std::to_string((int32)(healthScale * 100)) + "%";
		FrameworkElement::FindName<Noesis::TextBlock>("HEALTH_DISPLAY")->SetText(hpString.c_str());

		m_GUIState.Health = currentHealth;

		if (m_GUIState.Health <= 0)
			return false;
		else if(m_GUIState.Health <= 20)
			pBrush->SetColor(Color(255, 28, 0));
		else if(m_GUIState.Health <= 40)
			pBrush->SetColor(Color(255, 132, 0));
		else if(m_GUIState.Health <= 60)
			pBrush->SetColor(Color(255, 188, 0));
		else if(m_GUIState.Health <= 80)
			pBrush->SetColor(Color(141, 207, 0));		
		else if(m_GUIState.Health == 100)
			pBrush->SetColor(Color(0, 207, 56));
	}
	return true;
}

bool HUDGUI::UpdateScore()
{
	std::string scoreString;

	if (m_GUIState.Scores[0] != Match::GetScore(0))
	{
		m_GUIState.Scores[0] = Match::GetScore(0);

		scoreString = std::to_string(m_GUIState.Scores[0]);

		FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_1")->SetText(scoreString.c_str());
	}
	else if (m_GUIState.Scores[1] != Match::GetScore(1))
	{
		m_GUIState.Scores[1] = Match::GetScore(1);

		scoreString = std::to_string(m_GUIState.Scores[1]);

		FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_2")->SetText(scoreString.c_str());
	}
	return true;
}

bool HUDGUI::UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType)
{
	//Returns false if Out Of Ammo
	std::string ammoString;
	
	auto ammo = WeaponTypeAmmo.find(ammoType);
	if (ammo != WeaponTypeAmmo.end())
		ammoString = std::to_string(ammo->second.first) + "/" + std::to_string(ammo->second.second);
	else
		LOG_ERROR("Non-existing ammoType");


	if(ammoType == EAmmoType::AMMO_TYPE_WATER)
		FrameworkElement::FindName<TextBlock>("AMMUNITION_WATER_DISPLAY")->SetText(ammoString.c_str());

	if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
		FrameworkElement::FindName<TextBlock>("AMMUNITION_PAINT_DISPLAY")->SetText(ammoString.c_str());

	return true;
}

void HUDGUI::InitGUI()
{
	Noesis::Border* pHpRect = FrameworkElement::FindName<Noesis::Border>("HEALTH_RECT");

	m_GUIState.Health			= m_GUIState.MaxHealth;
	m_GUIState.AmmoCapacity		= 50;
	m_GUIState.Ammo				= m_GUIState.AmmoCapacity;

	m_GUIState.Scores.PushBack(Match::GetScore(0));
	m_GUIState.Scores.PushBack(Match::GetScore(1));

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);

	FrameworkElement::FindName<TextBlock>("AMMUNITION_DISPLAY")->SetText(ammoString.c_str());
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_1")->SetText("0");
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_2")->SetText("0");
}
