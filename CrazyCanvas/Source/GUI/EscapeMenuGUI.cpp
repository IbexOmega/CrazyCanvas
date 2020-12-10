#include "GUI/EscapeMenuGUI.h"

#include "States/MainMenuState.h"
#include "Engine/EngineConfig.h"

#include "Audio/AudioAPI.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "GUI/GUIHelpers.h"
#include "GUI/Core/GUIApplication.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Multiplayer/ClientHelper.h"

#include "NoesisPCH.h"

#include "Game/ECS/Systems/CameraSystem.h"

#include "Rendering/AARenderer.h"
#include "Rendering/RenderGraph.h"

using namespace LambdaEngine;
using namespace Noesis;


EscapeMenuGUI::EscapeMenuGUI()
{
	Noesis::GUI::LoadComponent(this, "EscapeMenuGUI.xaml");
}

EscapeMenuGUI::~EscapeMenuGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &EscapeMenuGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &EscapeMenuGUI::MouseButtonCallback);
}

void EscapeMenuGUI::InitGUI()
{
	// Main Grids
	m_pEscapeGrid = FrameworkElement::FindName<Grid>("EscapeGrid");

	m_WindowSize.x = CommonApplication::Get()->GetMainWindow()->GetWidth();
	m_WindowSize.y = CommonApplication::Get()->GetMainWindow()->GetHeight();

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &EscapeMenuGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &EscapeMenuGUI::MouseButtonCallback);

	m_pSettingsGUI = FindName<SettingsGUI>("SETTINGS_GUI");
	m_pSettingsGUI->InitGUI();
}

bool EscapeMenuGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// Escape
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonResumeClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonLeaveClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonExitClick);
	NS_CONNECT_EVENT(Button, IsVisibleChanged, OnSettingsClosed);

	return false;
}

void EscapeMenuGUI::ToggleEscapeMenu()
{
	EInputLayer currentInputLayer = Input::GetCurrentInputmode();

	if ((currentInputLayer == EInputLayer::GAME) || (currentInputLayer == EInputLayer::DEAD) && !m_EscapeMenuActive)
	{
		Input::PushInputMode(EInputLayer::GUI);
		m_MouseEnabled = !m_MouseEnabled;
		CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);

		m_EscapeMenuActive = true;
		m_pEscapeGrid->SetVisibility(Noesis::Visibility_Visible);
	}
}

void EscapeMenuGUI::OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	m_MouseEnabled = !m_MouseEnabled;
	m_EscapeMenuActive = false;
	CommonApplication::Get()->SetMouseVisibility(m_MouseEnabled);
	m_pEscapeGrid->SetVisibility(Noesis::Visibility_Collapsed);
	Input::PopInputMode();
}

void EscapeMenuGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	m_pEscapeGrid->SetVisibility(Noesis::Visibility_Collapsed);

	m_pSettingsGUI->ToggleSettings();
}

void EscapeMenuGUI::OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	ClientHelper::Disconnect("Left by choice");
	SetRenderStagesInactive();

	m_pEscapeGrid->SetVisibility(Noesis::Visibility_Collapsed);

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);

	Input::PopInputMode();
}

void EscapeMenuGUI::OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	CommonApplication::Get()->Terminate();
}

void EscapeMenuGUI::SetRenderStagesInactive()
{
	/*
	* Inactivate all rendering when entering main menu
	*OBS! At the moment, sleeping doesn't work correctly and needs a fix
	* */
	DisablePlaySessionsRenderstages();
}

bool EscapeMenuGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String keyStr = KeyToString(event.Key);

		m_pSetKeyButton->SetContent(keyStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = keyStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}
	else if (event.Key == EKey::KEY_ESCAPE)
	{
		ToggleEscapeMenu();

		return true;
	}

	return false;
}

bool EscapeMenuGUI::MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event)
{
	if (m_ListenToCallbacks)
	{
		LambdaEngine::String mouseButtonStr = ButtonToString(event.Button);

		m_pSetKeyButton->SetContent(mouseButtonStr.c_str());
		m_KeysToSet[m_pSetKeyButton->GetName()] = mouseButtonStr;

		m_ListenToCallbacks = false;
		m_pSetKeyButton = nullptr;

		return true;
	}

	return false;
}

void EscapeMenuGUI::OnSettingsClosed(Noesis::BaseComponent* pSender, const Noesis::DependencyPropertyChangedEventArgs& args)
{
	if (!m_pSettingsGUI->GetSettingsStatus() && m_EscapeMenuActive)
	{
		m_pEscapeGrid->SetVisibility(Noesis::Visibility_Visible);
	}
}
