#include "States/MainMenuState.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "GUI/GUIHelpers.h"

MainMenuState::~MainMenuState()
{
	m_MainMenuGUI.Reset();
	m_View.Reset();
}

void MainMenuState::Init()
{
	using namespace LambdaEngine;

	DisablePlaySessionsRenderstages();

	RenderGraphStructureDesc renderGraphStructure = {};

	m_MainMenuGUI = *new MainMenuGUI();
	m_View = Noesis::GUI::CreateView(m_MainMenuGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}

void MainMenuState::Tick(LambdaEngine::Timestamp delta)
{

}

void MainMenuState::FixedTick(LambdaEngine::Timestamp delta)
{
}
