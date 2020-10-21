#include "ECS/Systems/GUI/HUDSystem.h"

#include "Engine/EngineConfig.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"


HUDSystem::~HUDSystem()
{
	m_HUDGUI.Reset();
	m_View.Reset();
}

void HUDSystem::Init()
{
	using namespace LambdaEngine;

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	m_HUDGUI = *new HUDGUI("HUD.xaml");
	m_View = Noesis::GUI::CreateView(m_HUDGUI);

	LambdaEngine::GUIApplication::SetView(m_View);
}


void HUDSystem::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
	if (LambdaEngine::Input::GetMouseState().IsButtonPressed(LambdaEngine::EMouseButton::MOUSE_BUTTON_FORWARD))
		m_HUDGUI->UpdateAmmo();
}