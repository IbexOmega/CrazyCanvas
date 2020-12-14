#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/Button.h"
#include "NsGui/ComboBox.h"

#include "GUI/SettingsGUI.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#include <stack>

#include <functional>

class MainMenuGUI : public Noesis::Grid
{
public:
	MainMenuGUI();
	~MainMenuGUI();

private:
	friend class HUDGUI;

	// General
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	// StartGrid
	void OnButtonPlayClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// PlayGrid
	void OnButtonSandboxClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonMultiplayerClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonBenchmarkClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void OnSettingsClosed(Noesis::BaseComponent* pSender, const Noesis::DependencyPropertyChangedEventArgs& args);

private:
	void InitLevelSelect();
	void LevelSelectMouseEnter(Noesis::BaseComponent* pSender, const Noesis::MouseEventArgs& args);
	void LevelSelectMouseLeave(Noesis::BaseComponent* pSender, const Noesis::MouseEventArgs& args);
	void LevelSelectMousePressed(Noesis::BaseComponent* pSender, const Noesis::MouseButtonEventArgs& args);

	bool 	m_ListenToCallbacks			= false;

	int32	m_NewReflectionsSPP			= 0;

	Noesis::Grid*	m_pStartGrid		= nullptr;
	Noesis::Grid*	m_pPlayGrid			= nullptr;
	Noesis::Grid*	m_pLevelSelectGrid	= nullptr;

	SettingsGUI* m_pSettingsGUI = nullptr;

	LambdaEngine::TStack<Noesis::FrameworkElement*> m_ContextStack;

	NS_IMPLEMENT_INLINE_REFLECTION_(MainMenuGUI, Noesis::Grid);
};
