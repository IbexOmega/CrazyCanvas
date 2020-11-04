#include "States/LobbyState.h"

#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/RenderAPI.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"


LobbyState::~LobbyState()
{
	m_LobbyGUI.Reset();
	m_View.Reset();
}

void LobbyState::Init()
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

	m_LobbyGUI = *new LobbyGUI("Lobby.xaml");
	m_View = Noesis::GUI::CreateView(m_LobbyGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}

void LobbyState::Tick(LambdaEngine::Timestamp delta)
{

}

void LobbyState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_LobbyGUI->FixedTick(delta);
}
