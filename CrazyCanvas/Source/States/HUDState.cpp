#include "States/HUDState.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"


HUDState::~HUDState()
{
	m_HUDGUI.Reset();
	m_View.Reset();
}

void HUDState::Init()
{
	using namespace LambdaEngine;

	// Put unecessary renderstages to sleep in main menu

	/*RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DEFERRED_GEOMETRY_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("DIRL_SHADOWMAP", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("FXAA", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("POINTL_SHADOW", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SKYBOX_PASS", true);
	RenderSystem::GetInstance().SetRenderStageSleeping("SHADING_PASS", true);*/

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	m_HUDGUI = *new HUDGUI("HUD.xaml");
	m_View = Noesis::GUI::CreateView(m_HUDGUI);
	LambdaEngine::GUIApplication::SetView(m_View);
}


void HUDState::Tick(LambdaEngine::Timestamp delta)
{
	if (LambdaEngine::Input::GetMouseState().IsButtonPressed(LambdaEngine::EMouseButton::MOUSE_BUTTON_FORWARD))
		m_HUDGUI->UpdateAmmo();
}