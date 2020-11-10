#include "GUI/HUDGUI.h"
#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/GameOverGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "Game/State.h"

#include "Multiplayer/Packet/PacketType.h"

#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"
#include "Application/API/CommonApplication.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Match/Match.h"

#include <string>
#include "..\..\Include\GUI\HUDGUI.h"

using namespace LambdaEngine;
using namespace Noesis;

HUDGUI::HUDGUI() : 
	m_GUIState()
{
	Noesis::GUI::LoadComponent(this, "HUD.xaml");

	InitGUI();
}

HUDGUI::~HUDGUI()
{

}

bool HUDGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	UNREFERENCED_VARIABLE(pSource);
	UNREFERENCED_VARIABLE(pEvent);
	UNREFERENCED_VARIABLE(pHandler);

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
	Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();

	auto ammo = WeaponTypeAmmo.find(ammoType);

	if (ammo != WeaponTypeAmmo.end())
	{
		ammoString = std::to_string(ammo->second.first) + "/" + std::to_string(ammo->second.second);
		float ammoScale = (float)ammo->second.first / (float)ammo->second.second;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(ammoScale);
	}
	else
	{
		LOG_ERROR("Non-existing ammoType");
		return false;
	}


	if (ammoType == EAmmoType::AMMO_TYPE_WATER)
	{
		m_pWaterAmmoText->SetText(ammoString.c_str());
		m_pWaterAmmoRect->SetRenderTransform(scale);
	}
	else if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
	{
		m_pPaintAmmoText->SetText(ammoString.c_str());
		m_pPaintAmmoRect->SetRenderTransform(scale);
	}

	return true;
}

void HUDGUI::UpdateCountdown(uint8 countDownTime)
{
	CountdownGUI* pCountdownGUI = FindName<CountdownGUI>("COUNTDOWN");
	pCountdownGUI->UpdateCountdown(countDownTime);
}

void HUDGUI::DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal)
{
	Noesis::Ptr<Noesis::RotateTransform> rotateTransform = *new RotateTransform();

	glm::vec3 forwardDir = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
	glm::vec3 nor = glm::normalize(glm::vec3(collisionNormal.x, 0.0f, collisionNormal.z));

	float32 result = glm::dot(forwardDir, nor);
	float32 rotation = 0.0f;


	if (result > 0.99f)
	{
		rotation = 0.0f;
	}
	else if (result < -0.99f)
	{
		rotation = 180.0f;
	}
	else
	{
		glm::vec3 res = glm::cross(forwardDir, nor);

		rotation = glm::degrees(glm::acos(glm::dot(forwardDir, nor)));
		
		if (res.y > 0)
		{
			rotation *= -1;
		}
	}

	rotateTransform->SetAngle(rotation);
	m_pHitIndicatorGrid->SetRenderTransform(rotateTransform);

	DamageIndicatorGUI* pDamageIndicatorGUI = FindName<DamageIndicatorGUI>("DAMAGE_INDICATOR");
	pDamageIndicatorGUI->DisplayIndicator();
}

void HUDGUI::DisplayHitIndicator()
{
	EnemyHitIndicatorGUI* pEnemyHitIndicatorGUI = FindName<EnemyHitIndicatorGUI>("HIT_INDICATOR");
	pEnemyHitIndicatorGUI->DisplayIndicator();
}

void HUDGUI::DisplayGameOverGrid(uint8 winningTeamIndex)
{
	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Hidden);

	GameOverGUI* pGameOverGUI = FindName<GameOverGUI>("GAME_OVER");
	pGameOverGUI->InitGUI();
	pGameOverGUI->DisplayGameOverGrid(true);
	pGameOverGUI->SetWinningTeam(winningTeamIndex);
}

void HUDGUI::InitGUI()
{
	//Noesis::Border* pHpRect = FrameworkElement::FindName<Noesis::Border>("HEALTH_RECT");

	m_GUIState.Health			= m_GUIState.MaxHealth;
	m_GUIState.AmmoCapacity		= 50;
	m_GUIState.Ammo				= m_GUIState.AmmoCapacity;

	m_GUIState.Scores.PushBack(Match::GetScore(0));
	m_GUIState.Scores.PushBack(Match::GetScore(1));

	m_pWaterAmmoRect = FrameworkElement::FindName<Image>("WATER_RECT");
	m_pPaintAmmoRect = FrameworkElement::FindName<Image>("PAINT_RECT");

	m_pWaterAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_WATER_DISPLAY");
	m_pPaintAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_PAINT_DISPLAY");

	m_pHitIndicatorGrid = FrameworkElement::FindName<Grid>("DAMAGE_INDICATOR_GRID");

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);

	m_pWaterAmmoText->SetText(ammoString.c_str());
	m_pPaintAmmoText->SetText(ammoString.c_str());

	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_1")->SetText("0");
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_2")->SetText("0");

	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Visible);
	CommonApplication::Get()->SetMouseVisibility(false);
}
