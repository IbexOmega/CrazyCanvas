#include "GUI/EscapeMenuGUI.h"

#include "States/MainMenuState.h"
#include "Engine/EngineConfig.h"

#include "Audio/AudioAPI.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "GUI/GUIHelpers.h"
#include "GUI/Core/GUIApplication.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/ClientHelper.h"

#include "NoesisPCH.h"

#include "Game/ECS/Systems/CameraSystem.h"


using namespace LambdaEngine;
using namespace Noesis;


EscapeMenuGUI::EscapeMenuGUI()
{
	Noesis::GUI::LoadComponent(this, "EscapeMenuGUI.xaml");
}

EscapeMenuGUI::~EscapeMenuGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &EscapeMenuGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &EscapeMenuGUI::MouseButtonCallback);
}

void EscapeMenuGUI::InitGUI()
{
	// Main Grids
	m_pEscapeGrid = FrameworkElement::FindName<Grid>("EscapeGrid");
	m_pSettingsGrid = FrameworkElement::FindName<Grid>("SettingsGrid");
	m_pControlsGrid = FrameworkElement::FindName<Grid>("ControlsGrid");

	m_WindowSize.x = CommonApplication::Get()->GetMainWindow()->GetWidth();
	m_WindowSize.y = CommonApplication::Get()->GetMainWindow()->GetHeight();

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &EscapeMenuGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &EscapeMenuGUI::MouseButtonCallback);
}

bool EscapeMenuGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// Escape
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonResumeClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonExitClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonChangeControlsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnVolumeSliderChanged);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnFOVSliderChanged);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSetKey);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplyControlsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelControlsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnLookSensitivityChanged);

	return false;
}

void EscapeMenuGUI::ToggleEscapeMenu()
{
	EInputLayer currentInputLayer = Input::GetCurrentInputmode();

	if ((currentInputLayer == EInputLayer::GAME) || (currentInputLayer == EInputLayer::DEAD))
	{
		Input::PushInputMode(EInputLayer::GUI);
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);

		m_pEscapeGrid->SetVisibility(Noesis::Visibility_Visible);
		m_ContextStack.push(m_pEscapeGrid);
	}
	else if (currentInputLayer == EInputLayer::GUI)
	{
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
		Noesis::FrameworkElement* pElement = m_ContextStack.top();
		pElement->SetVisibility(Noesis::Visibility_Collapsed);
		m_ContextStack.pop();
		Input::PopInputMode();
	}
}

void EscapeMenuGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_ContextStack.pop();
	Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
	pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
}

void EscapeMenuGUI::OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	m_MouseEnabled = !m_MouseEnabled;
	CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Collapsed);
	m_ContextStack.pop();
	Input::PopInputMode();
}

void EscapeMenuGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pSettingsGrid);
}

void EscapeMenuGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	ClientHelper::Disconnect("Left by choice");
	SetRenderStagesInactive();

	Noesis::FrameworkElement* pElement = m_ContextStack.top();
	pElement->SetVisibility(Noesis::Visibility_Collapsed);
	m_ContextStack.pop();

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	Input::PopInputMode();
}

void EscapeMenuGUI::OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	CommonApplication::Get()->Terminate();
}

void EscapeMenuGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// NOTE: Current implementation does not allow RT toggle - code here if that changes
	// Ray Tracing
	// Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	// m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	// EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING, m_RayTracingEnabled);

	// Mesh Shader
	Noesis::CheckBox* pMeshShaderCheckBox = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	m_MeshShadersEnabled = pMeshShaderCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, m_MeshShadersEnabled);

	// Fullscreen toggle
	Noesis::CheckBox* pFullscreenCheckBox = FrameworkElement::FindName<CheckBox>("FullscreenCheckBox");
	bool previousState = m_FullscreenEnabled;
	m_FullscreenEnabled = pFullscreenCheckBox->GetIsChecked().GetValue();
	if (previousState != m_FullscreenEnabled)
		CommonApplication::Get()->GetMainWindow()->ToggleFullscreen();

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER, volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	//FOV
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV, CameraSystem::GetInstance().GetMainFOV());

	EngineConfig::WriteToFile();

	OnButtonBackClick(pSender, args);
}

void EscapeMenuGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	//FOV
	CameraSystem::GetInstance().SetMainFOV(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void EscapeMenuGUI::OnButtonChangeControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pControlsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pControlsGrid);
}

void EscapeMenuGUI::OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	// Update volume for easier changing of it. Do not save it however as that should
	// only be done when the user presses "Apply"

	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	AudioAPI::GetDevice()->SetMasterVolume(volume);
}

void EscapeMenuGUI::OnFOVSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pFOVSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	CameraSystem::GetInstance().SetMainFOV(pFOVSlider->GetValue());
}

void EscapeMenuGUI::OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// Starts listening to callbacks with specific button to be changed. This action is deferred to
	// the callback functions of KeyboardCallback and MouseButtonCallback.

	Noesis::Button* pCalledButton = static_cast<Noesis::Button*>(pSender);
	LambdaEngine::String buttonName = pCalledButton->GetName();

	m_pSetKeyButton = FrameworkElement::FindName<Button>(buttonName.c_str());
	m_ListenToCallbacks = true;
}

void EscapeMenuGUI::OnButtonApplyControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Go through all keys to set - and set them
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	for (auto& stringPair : m_KeysToSet)
	{
		InputActionSystem::ChangeKeyBinding(StringToAction(stringPair.first), stringPair.second);
	}
	m_KeysToSet.clear();

	InputActionSystem::SetLookSensitivity(m_LookSensitivityPercentageToSet);

	OnButtonBackClick(pSender, args);
}

void EscapeMenuGUI::OnButtonCancelControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Reset
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

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

void EscapeMenuGUI::OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pLookSensitivitySlider = reinterpret_cast<Noesis::Slider*>(pSender);

	m_LookSensitivityPercentageToSet = pLookSensitivitySlider->GetValue() / pLookSensitivitySlider->GetMaximum();
}

void EscapeMenuGUI::SetDefaultSettings()
{
	// Set inital volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(pVolumeSlider);
	float volume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER);
	pVolumeSlider->SetValue(volume * pVolumeSlider->GetMaximum());
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	//Set initial FOV
	Noesis::Slider* pFOVSlider = FrameworkElement::FindName<Slider>("FOVSlider");
	pFOVSlider->SetValue(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	SetDefaultKeyBindings();

	Noesis::Slider* pLookSensitivitySlider = FrameworkElement::FindName<Slider>("LookSensitivitySlider");
	pLookSensitivitySlider->SetValue(InputActionSystem::GetLookSensitivityPercentage() * pLookSensitivitySlider->GetMaximum());

	// NOTE: Current implementation does not allow RT toggle - code here if that changes
	// Ray Tracing Toggle
	// m_RayTracingEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);
	// CheckBox* pToggleRayTracing = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	// NS_ASSERT(pToggleRayTracing);
	// pToggleRayTracing->SetIsChecked(m_RayTracingEnabled);

	// Mesh Shader Toggle
	m_MeshShadersEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);
	ToggleButton* pToggleMeshShader = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);

	// Fullscreen
	m_FullscreenEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_FULLSCREEN);
	CheckBox* pToggleFullscreen = FrameworkElement::FindName<CheckBox>("FullscreenCheckBox");
	NS_ASSERT(pToggleFullscreen);
	pToggleFullscreen->SetIsChecked(m_FullscreenEnabled);
}

void EscapeMenuGUI::SetDefaultKeyBindings()
{
	TArray<EAction> actions = {
		// Movement
		EAction::ACTION_MOVE_FORWARD,
		EAction::ACTION_MOVE_BACKWARD,
		EAction::ACTION_MOVE_LEFT,
		EAction::ACTION_MOVE_RIGHT,
		EAction::ACTION_MOVE_JUMP,
		EAction::ACTION_MOVE_WALK,

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

void EscapeMenuGUI::SetRenderStagesInactive()
{
	/*
	* Inactivate all rendering when entering main menu
	*OBS! At the moment, sleeping doesn't work correctly and needs a fix
	* */
	DisablePlaySessionsRenderstages();
}

bool EscapeMenuGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
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
	else if (event.Key == EKey::KEY_ESCAPE)
	{
		ToggleEscapeMenu();

		return true;
	}

	return false;
}

bool EscapeMenuGUI::MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event)
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
