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

#include "Rendering/AARenderer.h"
#include "Rendering/RenderGraph.h"

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
	//m_pSettingsOverlay = FrameworkElement::FindName<Noesis::Grid>("SETTINGS_OVERLAY");
	m_pSettingsGrid = FrameworkElement::FindName<Noesis::Grid>("SettingsGrid");
	m_pControlsGrid = FrameworkElement::FindName<Noesis::Grid>("ControlsGrid");

	EventQueue::RegisterEventHandler<KeyPressedEvent>(this, &SettingsGUI::KeyboardCallback);
	EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(this, &SettingsGUI::MouseButtonCallback);

	SetDefaultSettings();
}

void SettingsGUI::ToggleSettings()
{
	m_SettingsActive = true;
	FrameworkElement::FindName<Noesis::UserControl>("SETTINGS_UC")->SetVisibility(Noesis::Visibility_Visible);
}

bool SettingsGUI::GetSettingsStatus() const
{
	return m_SettingsActive;
}

bool SettingsGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonApplySettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonCancelSettingsClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonChangeControlsClick);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnVolumeSliderChanged);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnMusicVolumeSliderChanged);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnFOVSliderChanged);
	NS_CONNECT_EVENT(Noesis::Slider, ValueChanged, OnReflectionsSPPSliderChanged);

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
		m_SettingsActive = false;
		FrameworkElement::FindName<Noesis::UserControl>("SETTINGS_UC")->SetVisibility(Noesis::Visibility_Collapsed);

	}
	else if (m_pControlsGrid->GetVisibility() == Noesis::Visibility_Visible)
	{
		m_pControlsGrid->SetVisibility(Noesis::Visibility_Collapsed);
		m_pSettingsGrid->SetVisibility(Noesis::Visibility_Visible);
	}
}

void SettingsGUI::OnButtonApplySettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;

	// NOTE: Current implementation does not allow RT toggle - code here if that changes
	// Ray Tracing
	// Noesis::CheckBox* pRayTracingCheckBox = FrameworkElement::FindName<CheckBox>("RayTracingCheckBox");
	// m_RayTracingEnabled = pRayTracingCheckBox->GetIsChecked().GetValue();
	// EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING, m_RayTracingEnabled);

	// Mesh Shader
	Noesis::CheckBox* pMeshShaderCheckBox = FrameworkElement::FindName<Noesis::CheckBox>("MeshShaderCheckBox");
	m_MeshShadersEnabled = pMeshShaderCheckBox->GetIsChecked().GetValue();
	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, m_MeshShadersEnabled);

	// Fullscreen toggle
	Noesis::CheckBox* pFullscreenCheckBox = FrameworkElement::FindName<Noesis::CheckBox>("FullscreenCheckBox");
	bool previousState = m_FullscreenEnabled;
	m_FullscreenEnabled = pFullscreenCheckBox->GetIsChecked().GetValue();
	if (previousState != m_FullscreenEnabled)
		CommonApplication::Get()->GetMainWindow()->ToggleFullscreen();

	EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_FULLSCREEN, m_FullscreenEnabled);

	// Volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Noesis::Slider>("VolumeSlider");
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER, volume);
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	//FOV
	EngineConfig::SetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV, CameraSystem::GetInstance().GetMainFOV());

	// AA
	Noesis::ComboBox* pAAComboBox = FrameworkElement::FindName<Noesis::ComboBox>("AAComboBox");
	LambdaEngine::String AAOption = static_cast<Noesis::TextBlock*>(pAAComboBox->GetSelectedItem())->GetText();
	EngineConfig::SetStringProperty(EConfigOption::CONFIG_OPTION_AA, AAOption);
	SetAA(pAAComboBox, AAOption);

	// Glossy
	{
		//Enabled
		Noesis::CheckBox* pGlossyReflectionsCheckbox = FrameworkElement::FindName<Noesis::CheckBox>("GlossyReflectionsCheckBox");
		bool glossyEnabled = pGlossyReflectionsCheckbox->GetIsChecked().GetValue();
		EngineConfig::SetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER, glossyEnabled);

		//SPP
		EngineConfig::SetIntProperty(EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP, m_NewReflectionsSPP);

		ChangeGlossySettings(glossyEnabled, m_NewReflectionsSPP);
	}

	EngineConfig::WriteToFile();

	OnButtonBackClick(pSender, args);
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

void SettingsGUI::OnMusicVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pVolumeSlider = reinterpret_cast<Noesis::Slider*>(pSender);
	float volume = pVolumeSlider->GetValue();
	float maxVolume = pVolumeSlider->GetMaximum();
	volume /= maxVolume;
	AudioAPI::GetDevice()->SetMusicVolume(volume);
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

void SettingsGUI::OnReflectionsSPPSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pReflectionsSPPSlider = reinterpret_cast<Noesis::Slider*>(pSender);

	m_NewReflectionsSPP = int32(pReflectionsSPPSlider->GetValue());

	ChangeGlossySettings(
		EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS),
		m_NewReflectionsSPP);
}

void SettingsGUI::SetAA(Noesis::ComboBox* pComboBox, const LambdaEngine::String& AAOption)
{
	if (AAOption == "NONE")
	{
		AARenderer::GetInstance()->SetAAMode(EAAMode::AAMODE_NONE);
		pComboBox->SetSelectedIndex(0);
	}
	else if (AAOption == "FXAA")
	{
		AARenderer::GetInstance()->SetAAMode(EAAMode::AAMODE_FXAA);
		pComboBox->SetSelectedIndex(1);
	}
	else if (AAOption == "TAA")
	{
		AARenderer::GetInstance()->SetAAMode(EAAMode::AAMODE_TAA);
		pComboBox->SetSelectedIndex(2);
	}
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
#ifdef LAMBDA_DEVELOPMENT
	if (Input::GetCurrentInputmode() == EInputLayer::DEBUG)
		return;
#endif

	//FOV
	CameraSystem::GetInstance().SetMainFOV(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	SetDefaultSettings();

	OnButtonBackClick(pSender, args);
}

void SettingsGUI::OnLookSensitivityChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args)
{
	Noesis::Slider* pLookSensitivitySlider = reinterpret_cast<Noesis::Slider*>(pSender);

	m_LookSensitivityPercentageToSet = pLookSensitivitySlider->GetValue() / pLookSensitivitySlider->GetMaximum();
}

void SettingsGUI::SetDefaultSettings()
{
	// Set inital volume
	Noesis::Slider* pVolumeSlider = FrameworkElement::FindName<Noesis::Slider>("VolumeSlider");
	NS_ASSERT(pVolumeSlider);
	float volume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MASTER);
	pVolumeSlider->SetValue(volume * pVolumeSlider->GetMaximum());
	AudioAPI::GetDevice()->SetMasterVolume(volume);

	// Set inital Music volume
	Noesis::Slider* pMusicVolumeSlider = FrameworkElement::FindName<Noesis::Slider>("MusicVolumeSlider");
	NS_ASSERT(pMusicVolumeSlider);
	float musicVolume = EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_VOLUME_MUSIC);
	pMusicVolumeSlider->SetValue(musicVolume * pMusicVolumeSlider->GetMaximum());
	AudioAPI::GetDevice()->SetMusicVolume(musicVolume);

	//Set initial FOV
	Noesis::Slider* pFOVSlider = FrameworkElement::FindName<Noesis::Slider>("FOVSlider");
	pFOVSlider->SetValue(EngineConfig::GetFloatProperty(EConfigOption::CONFIG_OPTION_CAMERA_FOV));

	//Set initial Glossy Toggle
	Noesis::CheckBox* pGlossyReflectionsCheckbox = FrameworkElement::FindName<Noesis::CheckBox>("GlossyReflectionsCheckBox");
	pGlossyReflectionsCheckbox->SetIsChecked(EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_GLOSSY_REFLECTIONS));

	//Set initial SPP
	Noesis::Slider* pReflectionsSPPSlider = FrameworkElement::FindName<Noesis::Slider>("ReflectionsSPPSlider");
	pReflectionsSPPSlider->SetValue(float32(EngineConfig::GetIntProperty(EConfigOption::CONFIG_OPTION_REFLECTIONS_SPP)));

	SetDefaultKeyBindings();

	Noesis::Slider* pLookSensitivitySlider = FrameworkElement::FindName<Noesis::Slider>("LookSensitivitySlider");
	pLookSensitivitySlider->SetValue(InputActionSystem::GetLookSensitivityPercentage() * pLookSensitivitySlider->GetMaximum());

	// Mesh Shader Toggle
	m_MeshShadersEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);
	Noesis::ToggleButton* pToggleMeshShader = FrameworkElement::FindName<Noesis::CheckBox>("MeshShaderCheckBox");
	NS_ASSERT(pToggleMeshShader);
	pToggleMeshShader->SetIsChecked(m_MeshShadersEnabled);

	// Fullscreen
	m_FullscreenEnabled = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_FULLSCREEN);
	Noesis::CheckBox* pToggleFullscreen = FrameworkElement::FindName<Noesis::CheckBox>("FullscreenCheckBox");
	NS_ASSERT(pToggleFullscreen);
	pToggleFullscreen->SetIsChecked(m_FullscreenEnabled);

	// AA option
	LambdaEngine::String AAOption = EngineConfig::GetStringProperty(EConfigOption::CONFIG_OPTION_AA);
	Noesis::ComboBox* pAAComboBox = FrameworkElement::FindName<Noesis::ComboBox>("AAComboBox");
	NS_ASSERT(pAAComboBox);
	SetAA(pAAComboBox, AAOption);
}

void SettingsGUI::SetDefaultKeyBindings()
{
	TArray<EAction> actions = {
		// Movement
		EAction::ACTION_MOVE_FORWARD,
		EAction::ACTION_MOVE_BACKWARD,
		EAction::ACTION_MOVE_LEFT,
		EAction::ACTION_MOVE_RIGHT,
		EAction::ACTION_MOVE_JUMP,
		EAction::ACTION_MOVE_WALK,

		// Attack
		EAction::ACTION_ATTACK_PRIMARY,
		EAction::ACTION_ATTACK_SECONDARY,
		EAction::ACTION_ATTACK_RELOAD,
	};

	for (EAction action : actions)
	{
		EKey key = InputActionSystem::GetKey(action);
		EMouseButton mouseButton = InputActionSystem::GetMouseButton(action);

		if (key != EKey::KEY_UNKNOWN)
		{
			FrameworkElement::FindName<Noesis::Button>(ActionToString(action))->SetContent(KeyToString(key));
		}
		else if (mouseButton != EMouseButton::MOUSE_BUTTON_UNKNOWN)
		{
			FrameworkElement::FindName<Noesis::Button>(ActionToString(action))->SetContent(ButtonToString(mouseButton));
		}
	}
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

