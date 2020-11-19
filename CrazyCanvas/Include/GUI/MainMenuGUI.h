#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/Button.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#include <stack>

class MainMenuGUI : public Noesis::Grid
{
public:
	MainMenuGUI();
	~MainMenuGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	// General

	// StartGrid
	void OnButtonPlayClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// PlayGrid
	void OnButtonSandboxClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonMultiplayerClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonBenchmarkClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	// Settings
	void OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonChangeControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);
	void OnFOVSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);

	// Controls
	void OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplyControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonCancelControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);

private:
	friend class HUDGUI;

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void SetDefaultSettings();
	void SetDefaultKeyBindings();
	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);

private:
	bool 	m_ListenToCallbacks			= false;
	Noesis::Button* m_pSetKeyButton		= nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;
	float32 m_LookSensitivityPercentageToSet = 0.0f;

	// bool	m_RayTracingEnabled			= false;
	bool	m_MeshShadersEnabled		= false;
	bool	m_FullscreenEnabled			= false;

	Noesis::Grid*	m_pStartGrid		= nullptr;
	Noesis::Grid*	m_pPlayGrid			= nullptr;
	Noesis::Grid*	m_pSettingsGrid 	= nullptr;
	Noesis::Grid*	m_pControlsGrid		= nullptr;

	GUID_Lambda m_MainMenuMusic;

	LambdaEngine::TStack<Noesis::FrameworkElement*> m_ContextStack;

	NS_IMPLEMENT_INLINE_REFLECTION_(MainMenuGUI, Noesis::Grid);
};
