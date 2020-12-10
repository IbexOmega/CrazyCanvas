#include "States/MainMenuState.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "GUI/GUIHelpers.h"

#include "Resources/ResourceCatalog.h"

#include "Application/API/CommonApplication.h"
#include "World/Player/PlayerActionSystem.h"
#include "Input/API/Input.h"

MainMenuState::~MainMenuState()
{
	using namespace LambdaEngine;

	m_MainMenuGUI.Reset();
	m_View.Reset();
}

void MainMenuState::Init()
{
	using namespace LambdaEngine;

	CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
	Input::PushInputLayer(EInputLayer::GUI);

	DisablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Play();

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
