#pragma once

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Grid.h>
#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/TextBlock.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>
#include <NoesisGUI/Include/NsGui/Button.h>

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

class SettingsGUI : public Noesis::UserControl
{
public:
	SettingsGUI();
	~SettingsGUI();

	void InitGUI();
	void ToggleSettings();

	bool GetSettingsStatus() const;
private:
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

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

	void SetDefaultSettings();
	void SetDefaultKeyBindings();

	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);

	NS_IMPLEMENT_INLINE_REFLECTION_(SettingsGUI, Noesis::UserControl, "CrazyCanvas.SettingsGUI");


private:
	bool 			m_ListenToCallbacks = false;
	Noesis::Button* m_pSetKeyButton		= nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;
	float32 m_LookSensitivityPercentageToSet = 0.0f;

	bool			m_MeshShadersEnabled	= false;
	bool			m_FullscreenEnabled		= false;
	bool			m_EscapeMenuEnabled		= false;

	bool			m_EscapeActive			= false;
	bool			m_MouseEnabled			= false;

	bool			m_SettingsActive		= false;

	Noesis::Grid* m_pSettingsGrid				= nullptr;
	Noesis::Grid* m_pControlsGrid				= nullptr;
};