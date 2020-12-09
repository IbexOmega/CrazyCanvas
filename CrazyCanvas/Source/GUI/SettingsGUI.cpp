#include "GUI/SettingsGUI.h"

#include "GUI/GUIHelpers.h"
#include "GUI/Core/GUIApplication.h"

#include "Application/API/CommonApplication.h"
#include "Application/API/Events/EventQueue.h"

#include "Audio/AudioAPI.h"

#include "Input/API/Input.h"
#include "Input/API/InputActionSystem.h"

#include "Game/ECS/Systems/CameraSystem.h"

#include "NoesisPCH.h"

using namespace LambdaEngine;

SettingsGUI::SettingsGUI()
{
	Noesis::GUI::LoadComponent(this, "SettingsGUI.xaml");
}

SettingsGUI::~SettingsGUI()
{
	EventQueue::UnregisterEventHandler<KeyPressedEvent>(this, &SettingsGUI::KeyboardCallback);
	EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(this, &SettingsGUI::MouseButtonCallback);
}

void SettingsGUI::InitGUI()
{
	m_pSettingsOverlay = FrameworkElement::FindName<Noesis::Grid>("SETTINGS_OVERLAY");
	m_pSettingsGrid = FrameworkElement::FindName<Noesis::Grid>("SettingsGrid");
	m_pControlsGrid = FrameworkElement::FindName<Noesis::Grid>("ControlsGrid");

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SettingsGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &SettingsGUI::MouseButtonCallback);

	SetDefaultSettings();
}

void SettingsGUI::ToggleSettings(bool isEscActive)
{
	if (isEscActive)
	{
		m_pSettingsOverlay->SetVisibility(Noesis::Visibility_Visible);
		m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	}
	else
	{
		m_pSettingsGrid->SetVisibility(Noesis::Visibility_Collapsed);
		m_pSettingsOverlay->SetVisibility(Noesis::Visibility_Collapsed);
	}
}

bool SettingsGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonChangeControlsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnVolumeSliderChanged);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnFOVSliderChanged);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonSetKey);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplyControlsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelControlsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnLookSensitivityChanged);

	return false;
}

void SettingsGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	if (m_pSettingsGrid->GetVisibility() == Noesis::Visibility_Visible)
	{
		m_pSettingsGrid->SetVisibility(Noesis::Visibility_Collapsed);
	}
	else if (m_pControlsGrid->GetVisibility() == Noesis::Visibility_Visible)
	{
		m_pControlsGrid->SetVisibility(Noesis::Visibility_Collapsed);
		m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	}
}

void SettingsGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
}

void SettingsGUI::OnButtonCancelSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	CameraSystem::GetInstance().SetMainFOV(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void SettingsGUI::OnButtonChangeControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	m_pSettingsGrid->SetVisibility(Noesis::Visibility_Collapsed);

	m_pControlsGrid->SetVisibility(Noesis::Visibility_Visible);
}

void SettingsGUI::OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Noesis::Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	AudioAPI::GetDevice()->SetMasterVolume(volume);
}

void SettingsGUI::OnFOVSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	Noesis::Slider* pFOVSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	CameraSystem::GetInstance().SetMainFOV(pFOVSlider->GetValue());
}

void SettingsGUI::OnButtonSetKey(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(args);

	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// Starts listening to callbacks with specific button to be changed. This action is deferred to
	// the callback functions of KeyboardCallback and MouseButtonCallback.

	Noesis::Button* pCalledButton = static_cast<Noesis::Button*>(pSender);
	LambdaEngine::String buttonName = pCalledButton->GetName();

	m_pSetKeyButton = FrameworkElement::FindName<Noesis::Button>(buttonName.c_str());
	m_ListenToCallbacks = true;
}

void SettingsGUI::OnButtonApplyControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	// Go through all keys to set - and set them
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	for (auto& stringPair : m_KeysToSet)
	{
		InputActionSystem::ChangeKeyBinding(StringToAction(stringPair.first), stringPair.second);
	}
	m_KeysToSet.clear();

	InputActionSystem::SetLookSensitivity(m_LookSensitivityPercentageToSet);

	OnButtonBackClick(pSender, args);
}

void SettingsGUI::OnButtonCancelControlsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
}

void SettingsGUI::OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
}

void SettingsGUI::SetDefaultSettings()
{
}

void SettingsGUI::SetDefaultKeyBindings()
{
}

bool SettingsGUI::KeyboardCallback(const LambdaEngine::KeyPressedEvent& event)
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
		return true;
	}

	return false;
}

bool SettingsGUI::MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event)
{
	return false;
}

