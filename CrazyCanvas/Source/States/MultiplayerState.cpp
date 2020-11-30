#include "States/MultiplayerState.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "GUI/GUIHelpers.h"

#include "Resources/ResourceCatalog.h"

#include "Application/API/CommonApplication.h"
#include "World/Player/PlayerActionSystem.h"
#include "Input/API/Input.h"

using namespace LambdaEngine;

MultiplayerState::~MultiplayerState()
{
	m_MultiplayerGUI.Reset();
	m_View.Reset();
}

void MultiplayerState::Init()
{
	using namespace LambdaEngine;

	CommonApplication::Get()->SetMouseVisibility(true);
	PlayerActionSystem::SetMouseEnabled(false);
	Input::PushInputMode(EInputLayer::GUI);

	DisablePlaySessionsRenderstages();
	ResourceManager::GetMusic(ResourceCatalog::MAIN_MENU_MUSIC_GUID)->Play();

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