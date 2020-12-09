#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "GUI/HUDGUI.h"
#include "GUI/CountdownGUI.h"
#include "GUI/PromptGUI.h"
#include "GUI/KillFeedGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/EnemyHitIndicatorGUI.h"
#include "GUI/GameOverGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "GUI/GUIHelpers.h"

#include "Game/State.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Match/Match.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "NoesisPCH.h"


#include "Application/API/Events/EventQueue.h"
#include "Application/API/CommonApplication.h"

#include "Match/Match.h"

#include "Lobby/PlayerManagerClient.h"
#include "Teams/TeamHelper.h"

#include "Game/ECS/Systems/CameraSystem.h"

#include <string>

using namespace LambdaEngine;
using namespace Noesis;

HUDGUI::HUDGUI() :
	m_GUIState()
{
	Noesis::GUI::LoadComponent(this, "HUD.xaml");

	InitGUI();

	m_pKillFeedGUI	= FindName<KillFeedGUI>("KILL_FEED");
	m_pKillFeedGUI->InitGUI();

	m_pEscMenuGUI	= FindName<EscapeMenuGUI>("ESC_MENU_GUI");
	m_pEscMenuGUI->InitGUI();

	m_pScoreBoardGUI = FindName<ScoreBoardGUI>("SCORE_BOARD_GUI");
	m_pScoreBoardGUI->InitGUI();
}

HUDGUI::~HUDGUI()
{
}

void HUDGUI::FixedTick(LambdaEngine::Timestamp delta)
{

	UpdateKillFeedTimer(delta);
	UpdateScore();

	if (m_IsReloading)
	{
		AnimateReload(float32(delta.AsSeconds()));
	}
}

void HUDGUI::AnimateReload(const float32 timePassed)
{
	/*Noesis::ScaleTransform* pWaterScale = (ScaleTransform*)m_pWaterAmmoRect->GetRenderTransform();
	Noesis::ScaleTransform* pPaintScale = (ScaleTransform*)m_pPaintAmmoRect->GetRenderTransform();

	pWaterScale->SetScaleX(glm::clamp<float>(pWaterScale->GetScaleX() + m_WaterAmmoFactor * timePassed, 0.0f, 1.0f));
	pPaintScale->SetScaleX(glm::clamp<float>(pPaintScale->GetScaleX() + m_PaintAmmoFactor * timePassed, 0.0f, 1.0f));

	m_pWaterAmmoRect->SetRenderTransform(pWaterScale);
	m_pPaintAmmoRect->SetRenderTransform(pPaintScale);*/
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
		Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();

		float healthScale = (float)currentHealth / (float)m_GUIState.MaxHealth;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(healthScale);
		m_pHealthRect->SetRenderTransform(scale);

		std::string hpString = std::to_string((int32)(healthScale * 100)) + "%";
		FrameworkElement::FindName<Noesis::TextBlock>("HEALTH_DISPLAY")->SetText(hpString.c_str());

		m_GUIState.Health = currentHealth;
	}
	return true;
}

bool HUDGUI::UpdateScore()
{
	std::string scoreString;
	uint32 team1Score = Match::GetScore(1);
	uint32 team2Score = Match::GetScore(2);

	if (team1Score > 5 || team2Score > 5)
		return true;

	// poor solution to handle bug if Match being reset before entering

	if (m_GUIState.Scores[0] != team1Score && team1Score != 0)
	{
		m_GUIState.Scores[0] = team1Score;

		m_pTeam1Score->SetText(std::to_string(team1Score).c_str());
	}
	else if (m_GUIState.Scores[1] != team2Score && team2Score != 0)
	{
		m_GUIState.Scores[1] = team2Score;

		m_pTeam2Score->SetText(std::to_string(team2Score).c_str());
	}

	return true;
}

bool HUDGUI::UpdateAmmo(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, EAmmoType ammoType)
{
	//Returns false if Out Of Ammo

	std::string ammoString;
	auto ammo = WeaponTypeAmmo.find(ammoType);
	/*Noesis::Ptr<Noesis::ScaleTransform> scale = *new ScaleTransform();
	float ammoScale = 0.0f;
	*/
	if (ammo != WeaponTypeAmmo.end())
	{
		ammoString = std::to_string(ammo->second.first) + "/" + std::to_string(ammo->second.second);
		/*ammoScale = (float)ammo->second.first / (float)ammo->second.second;
		scale->SetCenterX(0.0);
		scale->SetCenterY(0.0);
		scale->SetScaleX(ammoScale);*/

		if (ammoType == EAmmoType::AMMO_TYPE_WATER)
		{
			m_GUIState.WaterAmmo = ammo->second.first;

			m_pWaterAmmoText->SetText(ammoString.c_str());
			//m_pWaterAmmoRect->SetRenderTransform(scale);
		}
		else if (ammoType == EAmmoType::AMMO_TYPE_PAINT)
		{
			m_GUIState.PaintAmmo = ammo->second.first;

			m_pPaintAmmoText->SetText(ammoString.c_str());
			//m_pPaintAmmoRect->SetRenderTransform(scale);
		}
	}
	else
	{
		LOG_ERROR("Non-existing ammoType");
		return false;
	}

	return true;
}

void HUDGUI::Reload(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo, bool isReloading)
{
	m_IsReloading = isReloading;

	if (m_IsReloading)
	{
		for (auto& ammo : WeaponTypeAmmo)
		{
			float scale = (float)ammo.second.first / (float)ammo.second.second;

			if (ammo.first == EAmmoType::AMMO_TYPE_WATER)
				m_WaterAmmoFactor = (1.0f - scale) / m_ReloadAnimationTime;
			else if (ammo.first == EAmmoType::AMMO_TYPE_PAINT)
				m_PaintAmmoFactor = (1.0f - scale) / m_ReloadAnimationTime;
		}
	}
	else
	{
		std::string ammoString = std::to_string(25) + "/" + std::to_string(25);
		/*Noesis::Ptr<Noesis::ScaleTransform> scaleTransform = *new ScaleTransform();
		scaleTransform->SetCenterX(0.0);
		scaleTransform->SetCenterY(0.0);
		scaleTransform->SetScaleX(1.0f);*/

		m_pWaterAmmoText->SetText(ammoString.c_str());
		//m_pWaterAmmoRect->SetRenderTransform(scaleTransform);

		m_pPaintAmmoText->SetText(ammoString.c_str());
		//m_pPaintAmmoRect->SetRenderTransform(scaleTransform);
	}
}

void HUDGUI::AbortReload(const std::unordered_map<EAmmoType, std::pair<int32, int32>>& WeaponTypeAmmo)
{
	m_IsReloading = false;
	m_ReloadAnimationTime = 2.0f;
	CancelSmallPrompt();

	/*
	Noesis::Ptr<Noesis::ScaleTransform> waterScaleTransform = *new ScaleTransform();
	Noesis::Ptr<Noesis::ScaleTransform> paintScaleTransform = *new ScaleTransform();


	for (auto& ammo : WeaponTypeAmmo)
	{
		float scale = (float)ammo.second.first / (float)ammo.second.second;
		if (ammo.first == EAmmoType::AMMO_TYPE_WATER)
		{
			waterScaleTransform->SetCenterX(0.0);
			waterScaleTransform->SetCenterY(0.0);
			waterScaleTransform->SetScaleX(scale);

			m_pWaterAmmoRect->SetRenderTransform(waterScaleTransform);
		}
		else if (ammo.first == EAmmoType::AMMO_TYPE_PAINT)
		{
			paintScaleTransform->SetCenterX(0.0);
			paintScaleTransform->SetCenterY(0.0);
			paintScaleTransform->SetScaleX(scale);

			m_pPaintAmmoRect->SetRenderTransform(paintScaleTransform);
		}
	}*/
}


void HUDGUI::UpdateCountdown(uint8 countDownTime)
{
	CountdownGUI* pCountdownGUI = FindName<CountdownGUI>("COUNTDOWN");
	pCountdownGUI->UpdateCountdown(countDownTime);
}

void HUDGUI::DisplayDamageTakenIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal, bool isFriendly)
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
	pDamageIndicatorGUI->DisplayIndicator(isFriendly);
}

void HUDGUI::DisplayHitIndicator()
{
	EnemyHitIndicatorGUI* pEnemyHitIndicatorGUI = FindName<EnemyHitIndicatorGUI>("HIT_INDICATOR");
	pEnemyHitIndicatorGUI->DisplayIndicator();
}

void HUDGUI::DisplayCarryFlagIndicator(Entity flagEntity, bool isCarrying)
{
	auto grid = m_ProjectedElements.find(flagEntity);

	if (isCarrying)
	{
		grid->second->SetVisibility(Visibility::Visibility_Hidden);

		m_pCarryFlagIndicator->SetVisibility(Visibility::Visibility_Visible);
		m_pCarryFlagIndicatorStoryBoard->Begin();
	}
	else
	{
		grid->second->SetVisibility(Visibility::Visibility_Visible);

		m_pCarryFlagIndicator->SetVisibility(Visibility::Visibility_Collapsed);
		m_pCarryingFlagResetStoryBoard->Begin();
	}

}

void HUDGUI::UpdateKillFeed(const LambdaEngine::String& killed, const LambdaEngine::String& killer, uint8 killedPlayerTeamIndex)
{
	m_pKillFeedGUI->AddToKillFeed(killed, killer, killedPlayerTeamIndex);
}

void HUDGUI::UpdateKillFeedTimer(LambdaEngine::Timestamp delta)
{
	m_pKillFeedGUI->UpdateFeedTimer(delta);
}

void HUDGUI::ProjectGUIIndicator(const glm::mat4& viewProj, const glm::vec3& worldPos, Entity entity, IndicatorTypeGUI indicatorType)
{
	Noesis::Ptr<Noesis::TranslateTransform> translation = *new Noesis::TranslateTransform();
	Noesis::Ptr<Noesis::RotateTransform> rotation = *new RotateTransform();
	Noesis::Ptr<Noesis::TransformGroup> transformGroup = *new Noesis::TransformGroup();

	if (indicatorType == IndicatorTypeGUI::PING_INDICATOR)
		rotation->SetAngle(45.0f);

	transformGroup->GetChildren()->Add(rotation);
	transformGroup->GetChildren()->Add(translation);

	const glm::vec4 clipSpacePos = viewProj * glm::vec4(worldPos, 1.0f);

	VALIDATE(clipSpacePos.w != 0);

	const glm::vec3 ndcSpacePos = glm::vec3(clipSpacePos.x, clipSpacePos.y, clipSpacePos.z) / clipSpacePos.w;
	const glm::vec2 windowSpacePos = glm::vec2(ndcSpacePos.x, -ndcSpacePos.y) * 0.5f * m_WindowSize;

	float32 vecLength = glm::distance(glm::vec2(0.0f), glm::vec2(ndcSpacePos.x, ndcSpacePos.y));

	if (clipSpacePos.z > 0)
	{
		translation->SetY(glm::clamp(windowSpacePos.y, (-m_WindowSize.y + 100) * 0.5f, (m_WindowSize.y - 100) * 0.5f));
		translation->SetX(glm::clamp(windowSpacePos.x, (-m_WindowSize.x + 100) * 0.5f, (m_WindowSize.x - 100) * 0.5f));

		if (indicatorType == IndicatorTypeGUI::FLAG_INDICATOR)
		{
			SetIndicatorOpacity(glm::clamp(vecLength, 0.1f, 1.0f), entity);
		}
	}
	else
	{
		if (-clipSpacePos.y > 0)
			translation->SetY((m_WindowSize.y - 100) * 0.5f);
		else
			translation->SetY((-m_WindowSize.y + 100) * 0.5f);

		translation->SetX(glm::clamp(-windowSpacePos.x, (-m_WindowSize.x + 100) * 0.5f, (m_WindowSize.x - 100) * 0.5f));
	}

	TranslateIndicator(transformGroup, entity);


}

void HUDGUI::SetWindowSize(uint32 width, uint32 height)
{
	m_WindowSize = glm::vec2(width, height);
}

void HUDGUI::ShowHUD(const bool isVisible)
{
	if(isVisible)
		FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Visible);
	else
		FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Hidden);
}

void HUDGUI::ShowNamePlate(const LambdaEngine::String& name, bool isLooking)
{
	((Noesis::TextBlock*)m_pLookAtGrid->GetChildren()->Get(0))->SetText(name.c_str());

	if (isLooking)
		m_pLookAtGrid->SetVisibility(Noesis::Visibility::Visibility_Visible);
	else
		m_pLookAtGrid->SetVisibility(Noesis::Visibility::Visibility_Collapsed);
}

ScoreBoardGUI* HUDGUI::GetScoreBoard() const
{
	return m_pScoreBoardGUI;
}

void HUDGUI::DisplayGameOverGrid(uint8 winningTeamIndex, PlayerPair& mostKills, PlayerPair& mostDeaths, PlayerPair& mostFlags)
{
	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Hidden);

	GameOverGUI* pGameOverGUI = FindName<GameOverGUI>("GAME_OVER");
	pGameOverGUI->InitGUI();
	pGameOverGUI->DisplayGameOverGrid(true);
	pGameOverGUI->SetWinningTeam(winningTeamIndex);

	pGameOverGUI->SetMostKillsStats((uint8)mostKills.first, mostKills.second->GetName());
	pGameOverGUI->SetMostDeathsStats((uint8)mostDeaths.first, mostDeaths.second->GetName());
	pGameOverGUI->SetMostFlagsStats((uint8)mostFlags.first, mostFlags.second->GetName());
}

void HUDGUI::DisplayPrompt(const LambdaEngine::String& promptMessage, bool isSmallPrompt, uint8 teamIndex)
{
	PromptGUI* pPromptGUI = nullptr;

	if (isSmallPrompt)
	{
		pPromptGUI = FindName<PromptGUI>("SMALLPROMPT");
		pPromptGUI->DisplaySmallPrompt(promptMessage);
	}
	else
	{
		pPromptGUI = FindName<PromptGUI>("PROMPT");
		pPromptGUI->DisplayPrompt(promptMessage, teamIndex);
	}
}

void HUDGUI::DisplaySpectateText(const LambdaEngine::String& name, bool isSpectating)
{
	if (isSpectating)
	{
		LambdaEngine::String spectateText = "Spectating " + name;
		m_pSpectatePlayerText->SetText(spectateText.c_str());
		m_pSpectatePlayerText->SetVisibility(Visibility::Visibility_Visible);
	}
	else
	{
		m_pSpectatePlayerText->SetText("");
		m_pSpectatePlayerText->SetVisibility(Visibility::Visibility_Collapsed);
	}
}

void HUDGUI::CancelSmallPrompt()
{
	PromptGUI* pPromptGUI = nullptr;
	pPromptGUI = FindName<PromptGUI>("SMALLPROMPT");
	pPromptGUI->CancelSmallPrompt();
}

void HUDGUI::InitGUI()
{
	m_GUIState.Health			= m_GUIState.MaxHealth;

	m_GUIState.Scores.PushBack(Match::GetScore(1));
	m_GUIState.Scores.PushBack(Match::GetScore(2));

	m_pWaterAmmoRect	= FrameworkElement::FindName<Image>("WATER_RECT");
	m_pPaintAmmoRect	= FrameworkElement::FindName<Image>("PAINT_RECT");
	m_pPaintDropRect	= FrameworkElement::FindName<Image>("PAINT_DROP");
	m_pHealthRect		= FrameworkElement::FindName<Image>("HEALTH_RECT");

	m_pWaterAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_WATER_DISPLAY");
	m_pPaintAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_PAINT_DISPLAY");

	m_pHitIndicatorGrid	= FrameworkElement::FindName<Grid>("DAMAGE_INDICATOR_GRID");
	m_pLookAtGrid = FrameworkElement::FindName<Grid>("LookAtGrid");
	m_pCarryFlagBorder = FrameworkElement::FindName<Border>("CarryFlagBorder");
	m_pCarryFlagIndicator = FrameworkElement::FindName<Grid>("CarryFlagGrid");
	
	m_pHUDGrid = FrameworkElement::FindName<Grid>("ROOT_CONTAINER");

	m_pSpectatePlayerText = FrameworkElement::FindName<TextBlock>("SPECTATE_TEXT");

	m_pCarryFlagIndicatorStoryBoard = FrameworkElement::FindResource<Storyboard>("CarryingFlagStoryBoard");
	m_pCarryingFlagResetStoryBoard = FrameworkElement::FindResource<Storyboard>("CarryingFlagResetStoryBoard");


	Noesis::Ptr<BitmapImage> bitmap = *new BitmapImage(Uri(TeamHelper::GetTeamImage(PlayerManagerClient::GetPlayerLocal()->GetTeam()).PaintAmmo.c_str()));

	{ // init CarryFlagIndicator and LookAtGrid colors

		Ptr<Noesis::RadialGradientBrush> gradientBrush = *new Noesis::RadialGradientBrush();
		const glm::vec3& teamGradientColor = TeamHelper::GetTeamColor(PlayerManagerClient::GetPlayerLocal()->GetTeam() == 1 ? 2 : 1);
		Noesis::Color gradientColor(teamGradientColor.r, teamGradientColor.g, teamGradientColor.b);
		Ptr<Noesis::GradientStopCollection> gStops = *new GradientStopCollection();
		Ptr<Noesis::GradientStop> teamGradientStopColor = *new GradientStop();
		Ptr<Noesis::GradientStop> teamGradientStop = *new GradientStop();

		teamGradientStopColor->SetColor(gradientColor);
		teamGradientStopColor->SetOffset(1.0f);
		gStops->Add(teamGradientStopColor);
		gStops->Add(teamGradientStop);
		gradientBrush->SetGradientStops(gStops);

		m_pCarryFlagBorder->SetBackground(gradientBrush);


		Ptr<Noesis::SolidColorBrush> brush = *new Noesis::SolidColorBrush();
		const glm::vec3& teamColor = TeamHelper::GetTeamColor(PlayerManagerClient::GetPlayerLocal()->GetTeam());
		Noesis::Color color(teamColor.r, teamColor.g, teamColor.b);

		brush->SetColor(color);
		brush->SetOpacity(0.25f);

		m_pLookAtGrid->SetBackground(brush);

	}

	m_pPaintAmmoRect->SetSource(bitmap);

	InitScore();

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.WaterAmmo) + "/" + std::to_string((int)m_GUIState.WaterAmmoCapacity);

	m_pWaterAmmoText->SetText(ammoString.c_str());
	m_pPaintAmmoText->SetText(ammoString.c_str());

	FrameworkElement::FindName<Grid>("HUD_GRID")->SetVisibility(Noesis::Visibility_Visible);
	CommonApplication::Get()->SetMouseVisibility(false);

	m_WindowSize.x = CommonApplication::Get()->GetMainWindow()->GetWidth();
	m_WindowSize.y = CommonApplication::Get()->GetMainWindow()->GetHeight();
}

void HUDGUI::InitScore()
{
	m_pTeam1Score = FrameworkElement::FindName<Noesis::TextBlock>("SCORE_DISPLAY_TEAM_1");
	m_pTeam2Score = FrameworkElement::FindName<Noesis::TextBlock>("SCORE_DISPLAY_TEAM_2");

	Noesis::Ptr<Noesis::SolidColorBrush> pBrush1 = *new Noesis::SolidColorBrush();
	Noesis::Ptr<Noesis::SolidColorBrush> pBrush2 = *new Noesis::SolidColorBrush();

	glm::vec3 teamColor1 = TeamHelper::GetTeamColor(1);
	glm::vec3 teamColor2 = TeamHelper::GetTeamColor(2);
	Noesis::Color Color1(teamColor1.r, teamColor1.g, teamColor1.b);
	Noesis::Color Color2(teamColor2.r, teamColor2.g, teamColor2.b);

	pBrush1->SetColor(Color1);
	pBrush2->SetColor(Color2);

	m_pTeam1Score->SetForeground(pBrush1);
	m_pTeam2Score->SetForeground(pBrush2);

	m_pTeam1Score->SetText("0");
	m_pTeam2Score->SetText("0");
}

void HUDGUI::SetRenderStagesInactive()
{
	/*
	* Inactivate all rendering when entering main menu
	*OBS! At the moment, sleeping doesn't work correctly and needs a fix
	* */
	DisablePlaySessionsRenderstages();
}

void HUDGUI::CreateProjectedFlagGUIElement(Entity entity, uint8 localTeamIndex, uint8 teamIndex)
{
	Noesis::Ptr<Noesis::Grid> gridIndicator = *new Noesis::Grid();

	Noesis::Ptr<Noesis::Image> flagImage = *new Noesis::Image();
	Noesis::Ptr<Noesis::Ellipse> ellipseIndicator = *new Noesis::Ellipse();

	Noesis::Ptr<Noesis::TranslateTransform> translation = *new TranslateTransform();

	Noesis::Ptr<BitmapImage> bitmapFlag = *new BitmapImage(Uri("Roller.png"));

	flagImage->SetSource(bitmapFlag);

	translation->SetY(100.0f);
	translation->SetX(100.0f);

	gridIndicator->SetRenderTransform(translation);
	gridIndicator->SetRenderTransformOrigin(Noesis::Point(0.5f, 0.5f));

	Ptr<Noesis::SolidColorBrush> brush = *new Noesis::SolidColorBrush();
	Ptr<Noesis::SolidColorBrush> strokeBrush = *new Noesis::SolidColorBrush();

	strokeBrush->SetColor(Noesis::Color::Black());

	if (teamIndex != 0)
	{
		const glm::vec3& teamColor = TeamHelper::GetTeamColor(teamIndex);
		Noesis::Color color(teamColor.r, teamColor.g, teamColor.b);
		brush->SetColor(color);
	}
	else
		brush->SetColor(Noesis::Color::Green());

	flagImage->SetHeight(50);
	flagImage->SetWidth(50);

	gridIndicator->SetHeight(60);
	gridIndicator->SetWidth(60);

	ellipseIndicator->SetHeight(60);
	ellipseIndicator->SetWidth(60);

	ellipseIndicator->SetStroke(strokeBrush);
	ellipseIndicator->SetStrokeThickness(2);

	ellipseIndicator->SetFill(brush);

	gridIndicator->GetChildren()->Add(ellipseIndicator);
	gridIndicator->GetChildren()->Add(flagImage);

	m_ProjectedElements[entity] = gridIndicator;

	if (m_pHUDGrid->GetChildren()->Add(gridIndicator) == -1)
	{
		LOG_ERROR("Could not add Proj Element");
	}
}

void HUDGUI::CreateProjectedPingGUIElement(LambdaEngine::Entity entity)
{
	Noesis::Ptr<Noesis::Grid> gridIndicator = *new Noesis::Grid();

	Noesis::Ptr<Noesis::Border> pingBorderIndicator = *new Noesis::Border();

	Noesis::Ptr<Noesis::TranslateTransform> translation = *new Noesis::TranslateTransform();
	Noesis::Ptr<Noesis::RotateTransform> rotation = *new Noesis::RotateTransform();

	Noesis::Ptr<Noesis::TransformGroup> transformGroup = *new Noesis::TransformGroup();

	transformGroup->GetChildren()->Add(rotation);
	transformGroup->GetChildren()->Add(translation);

	translation->SetY(100.0f);
	translation->SetX(100.0f);

	rotation->SetCenterX(0.0f);
	rotation->SetCenterY(0.0f);

	rotation->SetAngle(45.0f);

	gridIndicator->SetRenderTransformOrigin(Noesis::Point(0.5f, 0.5f));

	gridIndicator->SetRenderTransform(transformGroup);

	Ptr<Noesis::SolidColorBrush> brush = *new Noesis::SolidColorBrush();
	Ptr<Noesis::SolidColorBrush> strokeBrush = *new Noesis::SolidColorBrush();

	strokeBrush->SetColor(Noesis::Color::Black());
	brush->SetColor(Noesis::Color::Orange());

	gridIndicator->SetHeight(25);
	gridIndicator->SetWidth(25);

	pingBorderIndicator->SetHeight(25);
	pingBorderIndicator->SetWidth(25);

	pingBorderIndicator->SetBorderBrush(strokeBrush);
	pingBorderIndicator->SetBorderThickness(2);

	pingBorderIndicator->SetBackground(brush);

	gridIndicator->GetChildren()->Add(pingBorderIndicator);

	m_ProjectedElements[entity] = gridIndicator;

	if (m_pHUDGrid->GetChildren()->Add(gridIndicator) == -1)
	{
		LOG_ERROR("Could not add Proj Element");
	}
}

void HUDGUI::RemoveProjectedGUIElement(LambdaEngine::Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	m_pHUDGrid->GetChildren()->Remove(indicator->second);

	m_ProjectedElements.erase(indicator->first);
}

void HUDGUI::TranslateIndicator(Noesis::TransformGroup* pTranslation, Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	indicator->second->SetRenderTransform(pTranslation);
}

void HUDGUI::SetIndicatorOpacity(float32 value, Entity entity)
{
	auto indicator = m_ProjectedElements.find(entity);
	VALIDATE(indicator != m_ProjectedElements.end())

	UIElement* pChildElement = indicator->second->GetChildren()->Get(0);
	Noesis::Ellipse* pTarget = reinterpret_cast<Noesis::Ellipse*>(pChildElement);
	pTarget->GetFill()->SetOpacity(value);
}
