#include "States/MainMenuState.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"


MainMenuState::~MainMenuState()
{
	int32 ref = m_MainMenuGUI->GetNumReferences();

	m_MainMenuGUI.Reset();
	m_View.Reset();
}

void MainMenuState::Init()
{
	using namespace LambdaEngine;

	// Put unecessary renderstages to sleep in main menu

	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true); 
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true); 

	RenderGraphStructureDesc renderGraphStructure = {};

	m_MainMenuGUI = *new MainMenuGUI("MainMenu.xaml");
	m_View = Noesis::GUI::CreateView(m_MainMenuGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}

void MainMenuState::Tick(LambdaEngine::Timestamp delta)
{
	
}
