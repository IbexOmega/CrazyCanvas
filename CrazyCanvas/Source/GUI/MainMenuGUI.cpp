#include "Application/API/CommonApplication.h"
#include "Audio/AudioAPI.h"
#include "Engine/EngineConfig.h"
#include "GUI/MainMenuGUI.h"
#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/StateManager.h"
#include "Game/State.h"
#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"
#include "States/BenchmarkState.h"
#include "States/PlaySessionState.h"
#include "States/SandboxState.h"
#include "States/MultiplayerState.h"

#include "Resources/ResourceManager.h"

#include "Application/API/Events/EventQueue.h"
#include "GUI/GUIHelpers.h"

#include "Game/ECS/Systems/CameraSystem.h"

#include "Rendering/AARenderer.h"
#include "Rendering/RenderGraph.h"

#include "World/LevelManager.h"

using namespace Noesis;
using namespace LambdaEngine;

MainMenuGUI::MainMenuGUI()
{
	GUI::LoadComponent(this, "MainMenu.xaml");

	// Main Grids
	m_pStartGrid				= FrameworkElement::FindName<Grid>("StartGrid");
	m_pPlayGrid					= FrameworkElement::FindName<Grid>("PlayGrid");
	m_pLevelSelectGrid			= FrameworkElement::FindName<Grid>("LevelSelectGrid");
	m_ContextStack.push(m_pStartGrid);

	m_pSettingsGUI = FindName<SettingsGUI>("SETTINGS_GUI");
	m_pSettingsGUI->InitGUI();

	InitLevelSelect();
}

MainMenuGUI::~MainMenuGUI()
{

}

bool MainMenuGUI::ConnectEvent(BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	// General
	NS_CONNECT_EVENT(Button, Click, OnButtonBackClick);

	// StartGrid
	NS_CONNECT_EVENT(Button, Click, OnButtonPlayClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonSettingsClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonExitClick);

	// PlayGrid
	NS_CONNECT_EVENT(Button, Click, OnButtonSandboxClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonMultiplayerClick);
	NS_CONNECT_EVENT(Button, Click, OnButtonBenchmarkClick);
	NS_CONNECT_EVENT(Button, IsVisibleChanged, OnSettingsClosed);

	return false;
}

/*
*
*	GENERAL BUTTONS
*
*/
void MainMenuGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_ContextStack.pop();
	Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
	pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
}

void MainMenuGUI::OnSettingsClosed(Noesis::BaseComponent* pSender, const DependencyPropertyChangedEventArgs& args)
{
	if (!m_pSettingsGUI->GetSettingsStatus())
	{
		Noesis::FrameworkElement* pCurrentElement = m_ContextStack.top();
		pCurrentElement->SetVisibility(Noesis::Visibility_Visible);
	}
}

/*
*
*	STARTGRID BUTTONS
*
*/
void MainMenuGUI::OnButtonPlayClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pPlayGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pPlayGrid);
}

void MainMenuGUI::OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pSettingsGUI->ToggleSettings();
}

void MainMenuGUI::OnButtonExitClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	CommonApplication::Get()->Terminate();
}

/*
*
*	PLAYGRID BUTTONS
*
*/
void MainMenuGUI::OnButtonSandboxClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	Noesis::FrameworkElement* pPrevElement = m_ContextStack.top();
	pPrevElement->SetVisibility(Noesis::Visibility_Collapsed);

	m_pLevelSelectGrid->SetVisibility(Noesis::Visibility_Visible);
	m_ContextStack.push(m_pLevelSelectGrid);

	// LambdaEngine::GUIApplication::SetView(nullptr);

	// PacketGameSettings settings;
	// settings.MapID		= 0;
	// settings.GameMode	= EGameMode::CTF_TEAM_FLAG;
	// State* pStartingState = DBG_NEW PlaySessionState(settings, true);
	// StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonMultiplayerClick(BaseComponent* pSender, const RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	LambdaEngine::GUIApplication::SetView(nullptr);

	State* pLobbyState = DBG_NEW MultiplayerState();
	StateManager::GetInstance()->EnqueueStateTransition(pLobbyState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::OnButtonBenchmarkClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputLayer() == EInputLayer::DEBUG)
		return;

	LambdaEngine::GUIApplication::SetView(nullptr);

	State* pStartingState = DBG_NEW BenchmarkState();
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}

void MainMenuGUI::InitLevelSelect()
{
	TArray<LevelManager::LevelDesc> levels = LevelManager::GetLevelDesc();

	for (auto& level : levels)
	{
		// Gets the levels that level manager has and add these to the level select grid with the correct name and index		// Grid
		Ptr<Grid> grid = *new Grid();
		ColumnDefinitionCollection* columnDef = grid->GetColumnDefinitions();
		Ptr<ColumnDefinition> col1 = *new ColumnDefinition();
		Ptr<ColumnDefinition> col2 = *new ColumnDefinition();
		col1->SetWidth(GridLength(1.f, GridUnitType::GridUnitType_Star));
		col2->SetWidth(GridLength(1.f, GridUnitType::GridUnitType_Auto));
		columnDef->Add(col1);
		columnDef->Add(col2);
		grid->SetHeight(60.f);
		grid->SetMargin(Thickness(0.f, 5.f, 0.f, 5.f));
		grid->MouseEnter() += MakeDelegate(this, &MainMenuGUI::LevelSelectMouseEnter);
		grid->MouseLeave() += MakeDelegate(this, &MainMenuGUI::LevelSelectMouseLeave);
		grid->MouseLeftButtonDown() += MakeDelegate(this, &MainMenuGUI::LevelSelectMousePressed);

		// Image
		Ptr<Image> image = *new Image();
		Ptr<BitmapImage> bitmap = *new BitmapImage(Uri(level.Thumbnail.c_str()));
		image->SetSource(bitmap);
		image->SetHorizontalAlignment(HorizontalAlignment::HorizontalAlignment_Left);
		grid->GetChildren()->Add(image);

		// Label
		Ptr<Label> label = *new Label();
		label->SetStyle(FrameworkElement::FindResource<Style>("LevelSelectLabelStyle"));
		label->SetContent(level.Name.c_str());
		label->SetHorizontalAlignment(HorizontalAlignment::HorizontalAlignment_Left);
		grid->GetChildren()->Add(label);

		FrameworkElement::FindName<Noesis::StackPanel>("LevelSelectStackPanel")->GetChildren()->Add(grid);
	}
}

void MainMenuGUI::LevelSelectMouseEnter(Noesis::BaseComponent* pSender, const Noesis::MouseEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	Grid* grid = static_cast<Grid*>(pSender);
	Ptr<SolidColorBrush> brush = *new SolidColorBrush();
	brush->SetColor(Color(0.1f, 0.1f, 0.1f));
	grid->SetBackground(brush);
}

void MainMenuGUI::LevelSelectMouseLeave(Noesis::BaseComponent* pSender, const Noesis::MouseEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	Grid* grid = static_cast<Grid*>(pSender);
	grid->SetBackground(static_cast<Grid*>(FrameworkElement::GetRoot())->GetBackground());
}

void MainMenuGUI::LevelSelectMousePressed(Noesis::BaseComponent* pSender, const Noesis::MouseButtonEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	Noesis::Grid* grid = static_cast<Noesis::Grid*>(pSender);
	Noesis::StackPanel* pnl = FrameworkElement::FindName<Noesis::StackPanel>("LevelSelectStackPanel");
	uint32 index = pnl->GetChildren()->IndexOf(grid);

	LambdaEngine::GUIApplication::SetView(nullptr);

	PacketGameSettings settings;
	settings.MapID		= index;
	settings.GameMode	= EGameMode::CTF_TEAM_FLAG;
	State* pStartingState = DBG_NEW PlaySessionState(settings, true);
	StateManager::GetInstance()->EnqueueStateTransition(pStartingState, STATE_TRANSITION::POP_AND_PUSH);
}