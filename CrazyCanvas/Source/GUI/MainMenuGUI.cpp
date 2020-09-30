#include "GUI/MainMenuGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/StateManager.h"
#include "Game/State.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"


MainMenuGUI::MainMenuGUI(const LambdaEngine::String& xamlFile)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
}

MainMenuGUI::~MainMenuGUI()
{
}



bool MainMenuGUI::ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler)
{
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButton1Click);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButton2Click);
	return false;
}

void MainMenuGUI::OnButton1Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	using namespace LambdaEngine;

	LOG_WARNING("PP1");

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", true);

	State* pStartingState = DBG_NEW PlaySessionState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::PUSH);
}

void MainMenuGUI::OnButton2Click(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	using namespace LambdaEngine;

	LOG_WARNING("PP2");

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", false);
	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", true);

	State* pStartingState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::PUSH);
}
