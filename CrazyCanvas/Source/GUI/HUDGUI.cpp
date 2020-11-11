#include "GUI/HUDGUI.h"
#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "GUI/GUIHelpers.h"

#include "Game/State.h"

#include "Multiplayer/Packet/PacketType.h"

#include "NoesisPCH.h"

#include "Application/API/Events/EventQueue.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Match/Match.h"

#include <string>

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

void HUDGUI::DisplayScoreboardMenu(bool visible)
{
	// Toggle to visible
	if (!m_ScoreboardVisible && visible)
	{
		m_ScoreboardVisible = true;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Visible);
	}
	// Toggle to hidden
	else if (m_ScoreboardVisible && !visible)
	{
		m_ScoreboardVisible = false;
		m_pScoreboardGrid->SetVisibility(Visibility::Visibility_Hidden);
	}
}

void HUDGUI::AddPlayer(const Player& newPlayer)
{
	Ptr<Grid> pGrid = *new Grid();

	pGrid->SetName(std::to_string(newPlayer.GetUID()).c_str());
	FrameworkElement::GetView()->GetContent()->RegisterName(pGrid->GetName(), pGrid);

	ColumnDefinitionCollection* pColumnCollection = pGrid->GetColumnDefinitions();
	AddColumnDefinition(pColumnCollection, 2.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 0.5f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);
	AddColumnDefinition(pColumnCollection, 1.0f, GridUnitType_Star);

	// Name is different than other labels
	Ptr<Label> pNameLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pNameLabel->SetContent(newPlayer.GetName().c_str());
	pNameLabel->SetForeground(pWhiteBrush);
	pNameLabel->SetFontSize(28.f);
	pNameLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	uint8 column = 0;
	pGrid->GetChildren()->Add(pNameLabel);
	pGrid->SetColumn(pNameLabel, column++);

	AddStatsLabel(pGrid, std::to_string(newPlayer.GetKills()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetDeaths()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsCaptured()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetFlagsDefended()), column++);
	AddStatsLabel(pGrid, std::to_string(newPlayer.GetPing()), column++);

	if (newPlayer.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else if (newPlayer.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Add(pGrid);
	}
	else
	{
		LOG_WARNING("[HUDGUI]: Unknown team on player \"%s\".\n\tUID: %lu\n\tTeam: %d",
			newPlayer.GetName().c_str(), newPlayer.GetUID(), newPlayer.GetTeam());
	}
}

void HUDGUI::RemovePlayer(const Player& player)
{
	Grid* pGrid = FrameworkElement::FindName<Grid>(std::to_string(player.GetUID()).c_str());

	if (!pGrid)
	{
		LOG_WARNING("[HUDGUI]: Tried to delete \"%s\", but could not find grid.\n\tUID: %lu",
			player.GetName().c_str(), player.GetUID());
		return;
	}

	if (player.GetTeam() == 0)
	{
		m_pBlueTeamStackPanel->GetChildren()->Remove(pGrid);
	}
	else if (player.GetTeam() == 1)
	{
		m_pRedTeamStackPanel->GetChildren()->Remove(pGrid);
	}
}

void HUDGUI::UpdatePlayerProperty(uint64 playerUID, EPlayerProperty property, const LambdaEngine::String& value)
{
	uint8 index = 0;
	switch (property)
	{
	case EPlayerProperty::PLAYER_PROPERTY_NAME:				index = 0; break;
	case EPlayerProperty::PLAYER_PROPERTY_KILLS:			index = 1; break;
	case EPlayerProperty::PLAYER_PROPERTY_DEATHS:			index = 2; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED:	index = 3; break;
	case EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED:	index = 4; break;
	case EPlayerProperty::PLAYER_PROPERTY_PING:				index = 5; break;
	default: LOG_WARNING("[HUDGUI]: Enum not supported"); return;
	}

	Grid* pGrid = FrameworkElement::FindName<Grid>(std::to_string(playerUID).c_str());
	if (!pGrid)
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", playerUID);
		return;
	}

	Label* pLabel = static_cast<Label*>(pGrid->GetChildren()->Get(index));
	pLabel->SetContent(value.c_str());
}

void HUDGUI::UpdateAllPlayerProperties(const Player& player)
{
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_NAME, player.GetName());
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_KILLS, std::to_string(player.GetKills()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_DEATHS, std::to_string(player.GetDeaths()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_CAPTURED, std::to_string(player.GetFlagsCaptured()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_FLAGS_DEFENDED, std::to_string(player.GetFlagsDefended()));
	UpdatePlayerProperty(player.GetUID(), EPlayerProperty::PLAYER_PROPERTY_PING, std::to_string(player.GetPing()));
}

void HUDGUI::AddStatsLabel(Noesis::Grid* pParentGrid, const LambdaEngine::String& content, uint32 column)
{
	Ptr<Label> pLabel = *new Label();

	Ptr<SolidColorBrush> pWhiteBrush = *new SolidColorBrush();
	pWhiteBrush->SetColor(Color::White());

	pLabel->SetContent(content.c_str());
	pLabel->SetForeground(pWhiteBrush);
	pLabel->SetHorizontalAlignment(HorizontalAlignment::HorizontalAlignment_Right);
	pLabel->SetVerticalAlignment(VerticalAlignment::VerticalAlignment_Bottom);
	pParentGrid->GetChildren()->Add(pLabel);
	pParentGrid->SetColumn(pLabel, column);
}

void HUDGUI::UpdatePlayerAliveStatus(uint64 UID, bool isAlive)
{
	Grid* pGrid = FrameworkElement::FindName<Grid>(std::to_string(UID).c_str());

	if (!pGrid)
	{
		LOG_WARNING("[HUDGUI]: Player with UID: &lu not found!", UID);
		return;
	}

	Label* nameLabel = static_cast<Label*>(pGrid->GetChildren()->Get(0));

	LOG_ERROR("[HUDGUI]: Name: %s", nameLabel->GetContent());

	SolidColorBrush* pBrush = static_cast<SolidColorBrush*>(nameLabel->GetForeground());
	if (isAlive)
	{
		pBrush->SetColor(Color::White());
		nameLabel->SetForeground(pBrush);
	}
	else
	{
		pBrush->SetColor(Color::LightGray());
		nameLabel->SetForeground(pBrush);
	}
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

	m_pHitIndicatorGrid	= FrameworkElement::FindName<Grid>("DAMAGE_INDICATOR_GRID");
	m_pScoreboardGrid	= FrameworkElement::FindName<Grid>("SCOREBOARD_GRID");

	m_pBlueTeamStackPanel	= FrameworkElement::FindName<StackPanel>("BLUE_TEAM_STACK_PANEL");
	m_pRedTeamStackPanel	= FrameworkElement::FindName<StackPanel>("RED_TEAM_STACK_PANEL");

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);

	m_pWaterAmmoText->SetText(ammoString.c_str());
	m_pPaintAmmoText->SetText(ammoString.c_str());

	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_1")->SetText("0");
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_2")->SetText("0");
}
