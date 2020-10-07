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



using namespace LambdaEngine;
using namespace Noesis;

MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	GUI::LoadComponent(this, xamlFile.c_str());

	m_pMainMenu = FrameworkElement::FindName<GroupBox>("MainMenuGroupBox");
	NS_ASSERT(m_pKeybindingsGroupBox != 0);
	m_pStatesGroupBox = FrameworkElement::FindName<GroupBox>("StatesGroupBox");
	NS_ASSERT(m_pKeybindingsGroupBox != 0);
	m_pSettingsGroupBox = FrameworkElement::FindName<GroupBox>("SettingsGroupBox");
	NS_ASSERT(m_pSettingsGroupBox != 0);
	m_pKeybindingsGroupBox = FrameworkElement::FindName<GroupBox>("KeybindingsGroupBox");
	NS_ASSERT(m_pKeybindingsGroupBox != 0);
	m_pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(m_pVolumeSlider != 0);

	// Set inital volume
	float volume = EngineConfig::GetFloatProperty("VolumeMasterInital");	
	m_pVolumeSlider->SetValue(volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// Init Keybindings
	FrameworkElement::FindName<TextBox>("CAM_FORWARD")->SetText(KeyToString(InputActionSystem::GetKey("CAM_FORWARD")));
	FrameworkElement::FindName<TextBox>("CAM_BACKWARD")->SetText(KeyToString(InputActionSystem::GetKey("CAM_BACKWARD")));
	FrameworkElement::FindName<TextBox>("CAM_LEFT")	->SetText(KeyToString(InputActionSystem::GetKey("CAM_LEFT")));
	FrameworkElement::FindName<TextBox>("CAM_RIGHT")->SetText(KeyToString(InputActionSystem::GetKey("CAM_RIGHT")));
	FrameworkElement::FindName<TextBox>("CAM_JUMP")	->SetText(KeyToString(InputActionSystem::GetKey("CAM_JUMP")));
	FrameworkElement::FindName<TextBox>("CAM_SPEED_MODIFIER")->SetText(KeyToString(InputActionSystem::GetKey("CAM_SPEED_MODIFIER")));
	
	m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
}

MainMenuGUI::~MainMenuGUI()
{
}

bool MainMenuGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Button, Click, OnButtonSingleplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonCrazyCanvasClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSandboxClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonBenchmarkClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonMultiplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonKeybindingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonExitClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonApplyClick);
	NS_CONNECT_EVENT(CheckBox, Click, OnRayTracingChecked);
	NS_CONNECT_EVENT(CheckBox, Click, OnMeshShadersChecked);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnVolumeSliderChanged);

	return false;
}

void MainMenuGUI::OnButtonSingleplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	m_ContextStack.push(m_pStatesGroupBox);
	ToggleView(m_pStatesGroupBox);
	m_pMainMenu->SetVisibility(Visibility::Visibility_Collapsed);
}


void MainMenuGUI::OnButtonCrazyCanvasClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW PlaySessionState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonSandboxClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesSleeping();
	State* pSandboxState = DBG_NEW SandboxState();
	StateManager::GetInstance()->EnqueueStateTransition(pSandboxState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonBenchmarkClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::GUIApplication::SetView(nullptr);

	SetRenderStagesSleeping();
	State* pBenchmarkState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pBenchmarkState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonMultiplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	//SetRenderStagesSleeping();

	State* pLobbyState = DBG_NEW LobbyState();
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonSettingsClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);
	m_ContextStack.push(m_pSettingsGroupBox);
	ToggleView(m_pSettingsGroupBox);
}

void MainMenuGUI::OnButtonKeybindingsClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);
	m_ContextStack.push(m_pKeybindingsGroupBox);
	ToggleView(m_pKeybindingsGroupBox);
}

void MainMenuGUI::OnButtonApplyClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	
	InputActionSystem::ChangeKeyBinding("CAM_FORWARD", StringToKey(FrameworkElement::FindName<TextBox>("CAM_FORWARD")->GetText()));
	InputActionSystem::ChangeKeyBinding("CAM_BACKWARD", StringToKey(FrameworkElement::FindName<TextBox>("CAM_BACKWARD")->GetText()));
	InputActionSystem::ChangeKeyBinding("CAM_LEFT", StringToKey(FrameworkElement::FindName<TextBox>("CAM_LEFT")->GetText()));
	InputActionSystem::ChangeKeyBinding("CAM_RIGHT", StringToKey(FrameworkElement::FindName<TextBox>("CAM_RIGHT")->GetText()));
	InputActionSystem::ChangeKeyBinding("CAM_JUMP", StringToKey(FrameworkElement::FindName<TextBox>("CAM_JUMP")->GetText()));
	InputActionSystem::ChangeKeyBinding("CAM_SPEED_MODIFIER", StringToKey(FrameworkElement::FindName<TextBox>("CAM_SPEED_MODIFIER")->GetText()));
	
	InputActionSystem::WriteToFile();
}

void MainMenuGUI::OnButtonBackClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (m_ContextStack.top() == m_pStatesGroupBox)
	{
		m_pStatesGroupBox->SetVisibility(Visibility::Visibility_Collapsed);
		m_pMainMenu->SetVisibility(Visibility::Visibility_Visible);
	}
	else
	{
		ToggleView(m_ContextStack.top());
	}
	m_ContextStack.pop();
}

void MainMenuGUI::OnButtonExitClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	CommonApplication::Get()->Terminate();
}

void MainMenuGUI::OnRayTracingChecked(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);

	ToggleButton* pFE = (ToggleButton*)args.source;
	m_RayTracingSleeping = pFE->GetIsChecked().GetValue();
}

void MainMenuGUI::OnMeshShadersChecked(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);

	ToggleButton* pFE = (ToggleButton*)args.source;
	m_MeshShadersSleeping = pFE->GetIsChecked().GetValue();
}

void MainMenuGUI::OnVolumeSliderChanged(BaseComponent* pSender, const RoutedPropertyChangedEventArgs<float>& args)
{
	AudioAPI::GetDevice()->SetMasterVolume(args.newValue);
}

void MainMenuGUI::SetRenderStagesSleeping()
{
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS",	false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP",			false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA",						false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS",				false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI",	true);

	if (m_RayTracingEnabled)
		RenderSystem::GetInstance().SetRenderStageSleeping("RAY_TRACING", m_RayTracingSleeping);

}

void MainMenuGUI::ToggleView(Noesis::FrameworkElement* element)
{
	if (element->GetVisibility() == Visibility::Visibility_Hidden || 
		element->GetVisibility() == Visibility::Visibility_Collapsed)
	{
		element->SetVisibility(Visibility::Visibility_Visible);
	}
	else
	{
		element->SetVisibility(Visibility::Visibility_Hidden);
	}
}

