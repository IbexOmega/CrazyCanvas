/*#include "States/HUDState.h"

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

	RenderSystem::GetInstance().SetRenderStageSleeping("RENDER_STAGE_NOESIS_GUI", false);

	m_HUDGUI	= *new HUDGUI("HUD.xaml");
	m_View		= Noesis::GUI::CreateView(m_HUDGUI);

	LambdaEngine::GUIApplication::SetView(m_View);
}


void HUDState::Tick(LambdaEngine::Timestamp delta)
{
	UNREFERENCED_VARIABLE(delta);
}*/