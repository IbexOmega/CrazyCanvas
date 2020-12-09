#include "Application/API/CommonApplication.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"
#include "GUI/MainMenuGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/MultiplayerState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/Events/EventQueue.h"
#include "GUI/GUIHelpers.h"

#include "Game/ECS/Systems/CameraSystem.h"

#include "Rendering/RenderGraph.h"

using namespace Noesis;
using namespace LambdaEngine;

MainMenuGUI::MainMenuGUI()
{
	GUI::LoadComponent(this, "MainMenu.xaml");

	// Main Grids
	m_pStartGrid		= FrameworkElement::FindName<Grid>("StartGrid");
	m_pPlayGrid			= FrameworkElement::FindName<Grid>("PlayGrid");
	m_pSettingsGrid		= FrameworkElement::FindName<Grid>("SettingsGrid");
	m_pControlsGrid		= FrameworkElement::FindName<Grid>("ControlsGrid");
	m_ContextStack.push(m_pStartGrid);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &MainMenuGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &MainMenuGUI::MouseButtonCallback);

	SetDefaultSettings();
}

MainMenuGUI::~MainMenuGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &MainMenuGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &MainMenuGUI::MouseButtonCallback);
}

bool MainMenuGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonBackClick);

	// StartGrid
	NS_CONNECT_EVENT(Button, Click, OnButtonPlayClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonExitClick);

	// PlayGrid
	NS_CONNECT_EVENT(Button, Click, OnButtonSandboxClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonMultiplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonBenchmarkClick);

	// SettingsGrid
	NS_CONNECT_EVENT(Button, Click, OnButtonChangeControlsClick);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnVolumeSliderChanged);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnMusicVolumeSliderChanged);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnFOVSliderChanged);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnReflectionsSPPSliderChanged);

	// Settings
	NS_CONNECT_EVENT(Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonCancelSettingsClick);

	// Key Bindings
	NS_CONNECT_EVENT(Button, Click, OnButtonSetKey);
	NS_CONNECT_EVENT(Button, Click, OnButtonApplyControlsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonCancelControlsClick);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnLookSensitivityChanged);

	return false;
}

/*
*
*	GENERAL BUTTONS
*
*/
void MainMenuGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_ContextStack.pop();
	Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
	pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
}

/*
*
*	STARTGRID BUTTONS
*
*/
void MainMenuGUI::OnButtonPlayClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pPlayGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pPlayGrid);
}

void MainMenuGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pSettingsGrid);
}

void MainMenuGUI::OnButtonExitClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	CommonApplication::Get()->Terminate();
}

/*
*
*	PLAYGRID BUTTONS
*
*/
void MainMenuGUI::OnButtonSandboxClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	PacketGameSettings settings;
	settings.MapID		= 0;
	settings.GameMode	= EGameMode::CTF_TEAM_FLAG;
	State* pStartingState = DBG_NEW PlaySessionState(settings, true);
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonMultiplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	State* pLobbyState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonBenchmarkClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	LambdaEngine::GUIApplication::SetView(nullptr);

	State* pStartingState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

/*
*
*	SETTINGS BUTTONS
*
*/
void MainMenuGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// NOTE: Current implementation does not allow RT toggle - code here if that changes
	// Ray Tracing
	// Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	// m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	// EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING, m_RayTracingEnabled);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

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

	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_FULLSCREEN, m_FullscreenEnabled);

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER, volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// Music Volume
	Noesis::Slider* pMusicVolumeSlider = FrameworkElement::FindName<Slider>("MusicVolumeSlider");
	float musicVolume = pMusicVolumeSlider->GetValue();
	float maxMusicVolume = pVolumeSlider->GetMaximum();
	musicVolume /= maxMusicVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MUSIC, musicVolume);
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->SetVolume(musicVolume);

	//FOV
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV, CameraSystem::GetInstance().GetMainFOV());

	// Glossy
	{
		//Enabled
		Noesis::CheckBox* pGlossyReflectionsCheckbox = FrameworkElement::FindName<CheckBox>("GlossyReflectionsCheckBox");
		bool glossyEnabled = pGlossyReflectionsCheckbox->GetIsChecked().GetValue();
		EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, glossyEnabled);

		//SPP
		EngineConfig::SetIntProperty(EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP, m_NewReflectionsSPP);

		ChangeGlossySettings(glossyEnabled, m_NewReflectionsSPP);
	}

	EngineConfig::WriteToFile();

	OnButtonBackClick(pSender, args);
}

void MainMenuGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	SetDefaultSettings();

	//FOV
	CameraSystem::GetInstance().SetMainFOV(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	//Glossy
	ChangeGlossySettings(
		EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS),
		EngineConfig::GetIntProperty(EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP));

	OnButtonBackClick(pSender, args);
}

void MainMenuGUI::OnButtonChangeControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pControlsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pControlsGrid);
}

void MainMenuGUI::OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	// Update volume for easier changing of it. Do not save it however as that should
	// only be done when the user presses "Apply"

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::Slider* pVolumeSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	AudioAPI::GetDevice()->SetMasterVolume(volume);
}

void MainMenuGUI::OnMusicVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pVolumeSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->SetVolume(volume);
}

void MainMenuGUI::OnFOVSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::Slider* pFOVSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	CameraSystem::GetInstance().SetMainFOV(pFOVSlider->GetValue());
}

void MainMenuGUI::OnReflectionsSPPSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pReflectionsSPPSlider = reinterpret_cast<Noesis::Slider*>(pSender);

	m_NewReflectionsSPP = int32(pReflectionsSPPSlider->GetValue());

	ChangeGlossySettings(
		EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS),
		m_NewReflectionsSPP);
}

/*
*
*	KEYBINDINGS BUTTONS
*
*/
void MainMenuGUI::OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	// Starts listening to callbacks with specific button to be changed. This action is deferred to
	// the callback functions of KeyboardCallback and MouseButtonCallback.

	Noesis::Button* pCalledButton = static_cast<Noesis::Button*>(pSender);
	LambdaEngine::String buttonName = pCalledButton->GetName();

	m_pSetKeyButton = FrameworkElement::FindName<Button>(buttonName.c_str());
	m_ListenToCallbacks = true;
}

void MainMenuGUI::OnButtonApplyControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	// Go through all keys to set - and set them
	for (auto& stringPair : m_KeysToSet)
	{
		InputActionSystem::ChangeKeyBinding(StringToAction(stringPair.first), stringPair.second);
	}
	m_KeysToSet.clear();

	InputActionSystem::SetLookSensitivity(m_LookSensitivityPercentageToSet);

	OnButtonBackClick(pSender, args);
}

void MainMenuGUI::OnButtonCancelControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

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

void MainMenuGUI::OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::Slider* pLookSensitivitySlider = reinterpret_cast<Noesis::Slider*>(pSender);

	m_LookSensitivityPercentageToSet = pLookSensitivitySlider->GetValue() / pLookSensitivitySlider->GetMaximum();
}

void MainMenuGUI::SetDefaultSettings()
{
	// Set inital Master volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(pVolumeSlider);
	float volume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER);
	pVolumeSlider->SetValue(volume * pVolumeSlider->GetMaximum());
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// Set inital Music volume
	Noesis::Slider* pMusicVolumeSlider = FrameworkElement::FindName<Slider>("MusicVolumeSlider");
	NS_ASSERT(pMusicVolumeSlider);
	float musicVolume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MUSIC);
	pMusicVolumeSlider->SetValue(musicVolume * pMusicVolumeSlider->GetMaximum());
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->SetVolume(musicVolume);

	//Set initial FOV
	Noesis::Slider* pFOVSlider = FrameworkElement::FindName<Slider>("FOVSlider");
	pFOVSlider->SetValue(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	//Set initial Glossy Toggle
	CheckBox* pGlossyReflectionsCheckbox = FrameworkElement::FindName<CheckBox>("GlossyReflectionsCheckBox");
	pGlossyReflectionsCheckbox->SetIsChecked(EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS));

	//Set initial SPP
	Noesis::Slider* pReflectionsSPPSlider = FrameworkElement::FindName<Slider>("ReflectionsSPPSlider");
	pReflectionsSPPSlider->SetValue(float32(EngineConfig::GetIntProperty(EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP)));

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
	CheckBox* pToggleMeshShader = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);

	// Fullscreen Toggle
	m_FullscreenEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_FULLSCREEN);
	CheckBox* pToggleFullscreen = FrameworkElement::FindName<CheckBox>("FullscreenCheckBox");
	NS_ASSERT(pToggleFullscreen);
	pToggleFullscreen->SetIsChecked(m_FullscreenEnabled);
}

void MainMenuGUI::SetDefaultKeyBindings()
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

bool MainMenuGUI::KeyboardCallback(const KeyPressedEvent& event)
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

bool MainMenuGUI::MouseButtonCallback(const MouseButtonClickedEvent& event)
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