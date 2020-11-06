#include "States/MultiplayerState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"


using namespace LambdaEngine;

MultiplayerState::~MultiplayerState()
{
	m_MultiplayerGUI.Reset();
	m_View.Reset();
}

void MultiplayerState::Init()
{
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS_MESH_PAINT", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true);

	m_MultiplayerGUI = *new MultiplayerGUI("Multiplayer.xaml");
	m_View = Noesis::GUI::CreateView(m_MultiplayerGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}

void MultiplayerState::Tick(LambdaEngine::Timestamp delta)
{

}

void MultiplayerState::FixedTick(LambdaEngine::Timestamp delta)
{
	m_MultiplayerGUI->FixedTick(delta);
}