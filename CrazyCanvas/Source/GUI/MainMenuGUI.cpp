#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/StateManager.h"
#include "Game/State.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Engine/EngineConfig.h"


MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	using namespace LambdaEngine;

	Noesis::GUI::LoadComponent(this, xamlFile.c_str());

	m_RayTracingEnabled = EngineConfig::GetBoolProperty("RayTracingEnabled");
}

MainMenuGUI::~MainMenuGUI()
{
}

bool MainMenuGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButton1Click);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButton2Click);
	NS_CONNECT_EVENT(Noesis::CheckBox, Click, OnRayTracingChecked);
	NS_CONNECT_EVENT(Noesis::CheckBox, Click, OnMeshShadersChecked);

	return false;
}

void MainMenuGUI::OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	using namespace LambdaEngine;
	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW PlaySessionState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	using namespace LambdaEngine;
	SetRenderStagesSleeping();

	State* pStartingState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}


void MainMenuGUI::OnRayTracingChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	LOG_WARNING("RT checked");

	Noesis::ToggleButton* pFE = (Noesis::ToggleButton*)args.source;
	m_RayTracingSleeping = pFE->GetIsChecked().GetValue();
}

void MainMenuGUI::OnMeshShadersChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	LOG_WARNING("MeshShaders checked");

	Noesis::ToggleButton* pFE = (Noesis::ToggleButton*)args.source;
	m_MeshShadersSleeping = pFE->GetIsChecked().GetValue();
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
