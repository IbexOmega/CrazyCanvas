#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"

class MainMenuGUI : public Noesis::Grid
{
public:
	MainMenuGUI(const LambdaEngine::String& xamlFile);
	~MainMenuGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSoruce, const char* pEvent, const char* pHandler) override;
	void OnButtonSingleplayerClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonMultiplayerClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonKeybindingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonApplyClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnRayTracingChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnMeshShadersChecked(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnVolumeSliderChanged(Noesis::BaseComponent* pSender, const Noesis::RoutedPropertyChangedEventArgs<float>& args);
private:
	void SetRenderStagesSleeping();
	void ToggleView(Noesis::FrameworkElement* element);

private:
	bool	m_RayTracingEnabled		= false;
	bool	m_RayTracingSleeping	= false;
	bool	m_MeshShadersSleeping	= false;

	Noesis::GroupBox*	m_pMainMenu;
	Noesis::GroupBox*	m_pSettingsGroupBox;
	Noesis::GroupBox*	m_pKeybindingsGroupBox;	
	Noesis::Slider*		m_pVolumeSlider;

	LambdaEngine::TStack<Noesis::FrameworkElement*> m_ContextStack;

	NS_IMPLEMENT_INLINE_REFLECTION_(MainMenuGUI, Noesis::Grid);
};