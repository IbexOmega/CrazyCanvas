#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "GUI/HUDGUI.h"
#include "GUI/CountdownGUI.h"
#include "GUI/DamageIndicatorGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "Match/Match.h"
#include "Multiplayer/ClientHelper.h"
#include "Multiplayer/Packet/PacketType.h"
#include "NoesisPCH.h"
#include "States/MainMenuState.h"


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
	//EventQueue::UnregisterEventHandler<ServerDiscoveredEvent>(this, &HUDGUI::OnLANServerFound);
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &HUDGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &HUDGUI::MouseButtonCallback);
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

	// Escape
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonResumeClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonExitClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonChangeKeyBindingsClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSetKey);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplyKeyBindingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelKeyBindingsClick);

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

bool HUDGUI::OpenEscapeMenu(const LambdaEngine::KeyPressedEvent& event)
{
	if (event.Key == EKey::KEY_ESCAPE && Input::GetCurrentInputmode() != EInputLayer::GUI)
	{
		Input::PushInputMode(EInputLayer::GUI);
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);

		m_pEscapeGrid->SetVisibility(Noesis::Visibility_Visible);
		m_ContextStack.push(m_pEscapeGrid);

		return true;
	}
	else if (event.Key == EKey::KEY_ESCAPE)
	{
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
		Noesis::FrameworkElement* pElement = m_ContextStack.top();
		pElement->SetVisibility(Noesis::Visibility_Hidden);
		m_ContextStack.pop();
		Input::PopInputMode();
	}

	return false;
}

void HUDGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_ContextStack.pop();
	Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
	pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
}

void HUDGUI::OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	m_MouseEnabled = !m_MouseEnabled;
	CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Hidden);
	m_ContextStack.pop();
	Input::PopInputMode();
}

void HUDGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pSettingsGrid);
}

void HUDGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ClientHelper::Disconnect("Daniel has the biggest penis");
	SetRenderStagesInactive();

	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Hidden);
	m_ContextStack.pop();

	//State* pMainMenuState = DBG_NEW MainMenuState();
	//StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void HUDGUI::OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	CommonApplication::Get()->Terminate();
}

void HUDGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Ray Tracing
	Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING, m_RayTracingEnabled);

	// Mesh Shader
	Noesis::CheckBox* pMeshShaderCheckBox = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	m_MeshShadersEnabled = pMeshShaderCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, m_MeshShadersEnabled);

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER, volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	EngineConfig::WriteToFile();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonChangeKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_pKeyBindingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pKeyBindingsGrid);
}

void HUDGUI::OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	// Starts listening to callbacks with specific button to be changed. This action is deferred to
	// the callback functions of KeyboardCallback and MouseButtonCallback.

	Noesis::Button* pCalledButton = static_cast<Noesis::Button*>(pSender);
	LambdaEngine::String buttonName = pCalledButton->GetName();

	m_pSetKeyButton = FrameworkElement::FindName<Button>(buttonName.c_str());
	m_ListenToCallbacks = true;
}

void HUDGUI::OnButtonApplyKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Go through all keys to set - and set them
	for (auto& stringPair : m_KeysToSet)
	{
		InputActionSystem::ChangeKeyBinding(StringToAction(stringPair.first), stringPair.second);
	}
	m_KeysToSet.clear();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::OnButtonCancelKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Reset
	for (auto& stringPair : m_KeysToSet)
	{
		EAction action = StringToAction(stringPair.first);
		EKey key = InputActionSystem::GetKey(action);
		EMouseButton mouseButton = InputActionSystem::GetMouseButton(action);

		if (key != EKey::KEY_UNKNOWN)
		{
			LambdaEngine::String keyStr = KeyToString(key);
			FrameworkElement::FindName<Button>(stringPair.first.c_str())->SetContent(keyStr.c_str());
		}
		if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			LambdaEngine::String mouseButtonStr = ButtonToString(mouseButton);
			FrameworkElement::FindName<Button>(stringPair.first.c_str())->SetContent(mouseButtonStr.c_str());
		}
	}
	m_KeysToSet.clear();

	OnButtonBackClick(pSender, args);
}

void HUDGUI::UpdateCountdown(uint8 countDownTime)
{
	CountdownGUI* pCountdownGUI = FindName<CountdownGUI>("COUNTDOWN");
	pCountdownGUI->UpdateCountdown(countDownTime);
}

void HUDGUI::DisplayHitIndicator(const glm::vec3& direction, const glm::vec3& collisionNormal)
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

void HUDGUI::InitGUI()
{
	//Noesis::Border* pHpRect = FrameworkElement::FindName<Noesis::Border>("HEALTH_RECT");

	m_GUIState.Health			= m_GUIState.MaxHealth;
	m_GUIState.AmmoCapacity		= 50;
	m_GUIState.Ammo				= m_GUIState.AmmoCapacity;

	m_GUIState.Scores.PushBack(Match::GetScore(0));
	m_GUIState.Scores.PushBack(Match::GetScore(1));

	m_pWaterAmmoRect = FrameworkElement::FindName<Border>("WATER_RECT");
	m_pPaintAmmoRect = FrameworkElement::FindName<Border>("PAINT_RECT");

	m_pWaterAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_WATER_DISPLAY");
	m_pPaintAmmoText = FrameworkElement::FindName<TextBlock>("AMMUNITION_PAINT_DISPLAY");

	m_pHitIndicatorGrid = FrameworkElement::FindName<Grid>("DAMAGE_INDICATOR_GRID");

	std::string ammoString;

	ammoString	= std::to_string((int)m_GUIState.Ammo) + "/" + std::to_string((int)m_GUIState.AmmoCapacity);

	m_pWaterAmmoText->SetText(ammoString.c_str());
	m_pPaintAmmoText->SetText(ammoString.c_str());

	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_1")->SetText("0");
	FrameworkElement::FindName<TextBlock>("SCORE_DISPLAY_TEAM_2")->SetText("0");

	// Main Grids
	m_pEscapeGrid			= FrameworkElement::FindName<Grid>("EscapeGrid");
	m_pSettingsGrid			= FrameworkElement::FindName<Grid>("SettingsGrid");
	m_pKeyBindingsGrid		= FrameworkElement::FindName<Grid>("KeyBindingsGrid");

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &HUDGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &HUDGUI::OpenEscapeMenu);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &HUDGUI::MouseButtonCallback);

	SetDefaultSettings();
}

void HUDGUI::SetDefaultSettings()
{
	// Set inital volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(pVolumeSlider);
	float volume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER);
	pVolumeSlider->SetValue(volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	SetDefaultKeyBindings();

	// Ray Tracing Toggle
	m_RayTracingEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);
	CheckBox* pToggleRayTracing = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	NS_ASSERT(pToggleRayTracing);
	pToggleRayTracing->SetIsChecked(m_RayTracingEnabled);

	// Mesh Shader Toggle
	m_MeshShadersEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);
	ToggleButton* pToggleMeshShader = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);
}

void HUDGUI::SetDefaultKeyBindings()
{
	TArray<EAction> actions = {
		// Movement
		EAction::ACTION_MOVE_FORWARD,
		EAction::ACTION_MOVE_BACKWARD,
		EAction::ACTION_MOVE_LEFT,
		EAction::ACTION_MOVE_RIGHT,
		EAction::ACTION_MOVE_JUMP,
		EAction::ACTION_MOVE_SPRINT,

		// Attack
		EAction::ACTION_ATTACK_PRIMARY,
		EAction::ACTION_ATTACK_SECONDARY,
		EAction::ACTION_ATTACK_RELOAD,
	};

	for (EAction action : actions)
	{
		EKey key = InputActionSystem::GetKey(action);
		EMouseButton mouseButton = InputActionSystem::GetMouseButton(action);

		if (key != EKey::KEY_UNKNOWN)
		{
			FrameworkElement::FindName<Button>(ActionToString(action))->SetContent(KeyToString(key));
		}
		else if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			FrameworkElement::FindName<Button>(ActionToString(action))->SetContent(ButtonToString(mouseButton));
		}
	}
}

void HUDGUI::SetRenderStagesInactive()
{
	/*
	* Inactivate all rendering when entering main menu
	*OBS! At the moment, sleeping doesn't work correctly and needs a fix
	* */
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",						true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",			true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP",					true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA",								true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW",						true);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_PARTICLE_RENDER",		true);
	RenderSystem::GetInstance().SetRenderStageSleeping("PARTICLE_COMBINE_PASS",				true);
	RenderSystem::GetInstance().SetRenderStageSleeping("PLAYER_PASS",						true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",						true);
	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING",						true);
}

bool HUDGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String keyStr = KeyToString(event.Key);

		m_pSetKeyButton->SetContent(keyStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = keyStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}

	return false;
}

bool HUDGUI::MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String mouseButtonStr = ButtonToString(event.Button);

		m_pSetKeyButton->SetContent(mouseButtonStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = mouseButtonStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}

	return false;
}
