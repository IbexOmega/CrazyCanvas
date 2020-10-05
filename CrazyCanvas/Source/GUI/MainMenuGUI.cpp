#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/StateManager.h"
#include "Game/State.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Engine/EngineConfig.h"
#include "Audio/AudioAPI.h"

using namespace LambdaEngine;
using namespace Noesis;

MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	GUI::LoadComponent(this, xamlFile.c_str());


	m_pSettingsGroupBox = FrameworkElement::FindName<GroupBox>("SettingsGroupBox");
	NS_ASSERT(m_pSettingsGroupBox != 0);
	m_pVolumeSlider = FrameworkElement::FindName<Slider>("VolumeSlider");
	NS_ASSERT(m_pVolumeSlider != 0);

	// Set inital volume
	float volume = EngineConfig::GetFloatProperty("VolumeMasterInital");	
	m_pVolumeSlider->SetValue(volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
}

MainMenuGUI::~MainMenuGUI()
{
}

bool MainMenuGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Button, Click, OnButtonSingleplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonMultiplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(CheckBox, Click, OnRayTracingChecked);
	NS_CONNECT_EVENT(CheckBox, Click, OnMeshShadersChecked);
	NS_CONNECT_EVENT(Slider, ValueChanged, OnVolumeSliderChanged);

	return false;
}

void MainMenuGUI::OnButtonSingleplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	using namespace LambdaEngine;
	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW PlaySessionState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonMultiplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	using namespace LambdaEngine;
	SetRenderStagesSleeping();

	/*State* pLobbyState = DBG_NEW LobbyState();
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);*/
}

void MainMenuGUI::OnButtonSettingsClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ToggleSettingsView();
}

void MainMenuGUI::OnButtonBackClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ToggleSettingsView();
}


void MainMenuGUI::OnButtonExitClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	using namespace LambdaEngine;
	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}



void MainMenuGUI::OnRayTracingChecked(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	LOG_WARNING("RT checked");

	ToggleButton* pFE = (ToggleButton*)args.source;
	m_RayTracingSleeping = pFE->GetIsChecked().GetValue();
}

void MainMenuGUI::OnMeshShadersChecked(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	LOG_WARNING("MeshShaders checked");

	ToggleButton* pFE = (ToggleButton*)args.source;
	m_MeshShadersSleeping = pFE->GetIsChecked().GetValue();
}

void MainMenuGUI::OnVolumeSliderChanged(BaseComponent* pSender, const RoutedPropertyChangedEventArgs<float>& args)
{
	AudioAPI::GetDevice()->SetMasterVolume(args.newValue);
	LOG_MESSAGE("Changed volume! %f", AudioAPI::GetDevice()->GetMasterVolume());
}

void MainMenuGUI::SetRenderStagesSleeping()
{
	using namespace LambdaEngine;

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

void MainMenuGUI::ToggleSettingsView()
{
	if (m_pSettingsGroupBox->GetVisibility() == Visibility::Visibility_Hidden)
	{
		m_pSettingsGroupBox->SetVisibility(Visibility::Visibility_Visible);
	}
	else
	{
		m_pSettingsGroupBox->SetVisibility(Visibility::Visibility_Hidden);
	}
}
