#include "Application/API/CommonApplication.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"
#include "GUI/MainMenuGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "Input/API/InputActionSystem.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/LobbyState.h"

#include "Application/API/Events/EventQueue.h"


using namespace Noesis;
using namespace LambdaEngine;

MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	GUI::LoadComponent(this, xamlFile.c_str());

	// Main Grids
	m_pStartGrid		= FrameworkElement::FindName<Grid>("StartGrid");
	m_pPlayGrid			= FrameworkElement::FindName<Grid>("PlayGrid");
	m_pSettingsGrid		= FrameworkElement::FindName<Grid>("SettingsGrid");
	m_pKeyBindingsGrid	= FrameworkElement::FindName<Grid>("KeyBindingsGrid");
	m_ContextStack.push(m_pStartGrid);

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &MainMenuGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &MainMenuGUI::MouseCallback);

	SetDefaultSettings();
}

MainMenuGUI::~MainMenuGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &MainMenuGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &MainMenuGUI::MouseCallback);
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
	NS_CONNECT_EVENT(Button, Click, OnButtonChangeKeyBindingsClick);

	// Settings
	NS_CONNECT_EVENT(Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonCancelSettingsClick);

	// Key Bindings
	NS_CONNECT_EVENT(Button, Click, OnButtonSetForwardKey);

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

	Noesis::FrameworkElement* prevElement = m_ContextStack.top();
	prevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_ContextStack.pop();
	Noesis::FrameworkElement* currentElement = m_ContextStack.top();
	currentElement->SetVisibility(Noesis::Visibility_Visible);
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

	Noesis::FrameworkElement* prevElement = m_ContextStack.top();
	prevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_pPlayGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pPlayGrid);
}

void MainMenuGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Noesis::FrameworkElement* prevElement = m_ContextStack.top();
	prevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pSettingsGrid);
}

void MainMenuGUI::OnButtonExitClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

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

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW PlaySessionState(true, IPAddress::LOOPBACK);
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonMultiplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	State* pLobbyState = DBG_NEW LobbyState();
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonBenchmarkClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesSleeping();

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
	// Ray Tracing
	Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty("RayTracingEnabled", m_RayTracingEnabled);

	// Mesh Shader
	Noesis::CheckBox* pMeshShaderCheckBox = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	m_MeshShadersEnabled = pMeshShaderCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty("MeshShadersEnabled", m_MeshShadersEnabled);

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty("VolumeMasterInital", volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// TODO: EngineConfig does not set the expected values

	OnButtonBackClick(pSender, args);
}

void MainMenuGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void MainMenuGUI::OnButtonChangeKeyBindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	Noesis::FrameworkElement* prevElement = m_ContextStack.top();
	prevElement->SetVisibility(Noesis::Visibility_Hidden);

	m_pKeyBindingsGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pKeyBindingsGrid);
}

/*
*
*	KEYBINDINGS BUTTONS
*
*/
void MainMenuGUI::OnButtonSetForwardKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	SetKey("CAM_FORWARD");
}

/*
*
*	HELPER FUNCTIONS
*
*/
void MainMenuGUI::SetRenderStagesSleeping()
{
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT",	false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP",					false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA",								false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("PLAYER_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", true);

	RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING", !m_RayTracingEnabled);

}

void MainMenuGUI::SetDefaultSettings()
{
	// Set inital volume
	Noesis::Slider* volumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(volumeSlider);
	float volume = EngineConfig::GetFloatProperty("VolumeMasterInital");
	volumeSlider->SetValue(volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// Init Keybindings
	FrameworkElement::FindName<Button>("CAM_FORWARD")->SetContent(KeyToString(InputActionSystem::GetKey("CAM_FORWARD")));

	FrameworkElement::FindName<TextBox>("CAM_BACKWARD")->SetText(KeyToString(InputActionSystem::GetKey("CAM_BACKWARD")));
	FrameworkElement::FindName<TextBox>("CAM_LEFT")	->SetText(KeyToString(InputActionSystem::GetKey("CAM_LEFT")));
	FrameworkElement::FindName<TextBox>("CAM_RIGHT")->SetText(KeyToString(InputActionSystem::GetKey("CAM_RIGHT")));
	FrameworkElement::FindName<TextBox>("CAM_JUMP")	->SetText(KeyToString(InputActionSystem::GetKey("CAM_JUMP")));
	FrameworkElement::FindName<TextBox>("CAM_SPEED_MODIFIER")->SetText(KeyToString(InputActionSystem::GetKey("CAM_SPEED_MODIFIER")));

	// TODO: Get attack buttons (primary shoot, secondary shoot, reload) when they are available from the config file

	// Ray Tracing Toggle
	m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
	CheckBox* pToggleRayTracing = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	NS_ASSERT(pToggleRayTracing);
	pToggleRayTracing->SetIsChecked(m_RayTracingEnabled);

	// Mesh Shader Toggle
	m_MeshShadersEnabled = EngineConfig::GetBoolProperty("MeshShadersEnabled");
	ToggleButton* pToggleMeshShader = FrameworkElement::FindName<CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);
}

bool MainMenuGUI::KeyboardCallback(const KeyPressedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		m_ListenToCallbacks = false;
	}
}

bool MainMenuGUI::MouseCallback(const MouseButtonClickedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		m_ListenToCallbacks = false;
	}
}

void MainMenuGUI::SetKey(const char* buttonName)
{
	Noesis::Button* button = FrameworkElement::FindName<Button>(buttonName);


}