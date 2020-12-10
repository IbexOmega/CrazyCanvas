#pragma once

#include "Containers/String.h"
#include "Containers/TStack.h"
#include "Containers/THashTable.h"

#include "LambdaEngine.h"

#include "GUI/SettingsGUI.h"

#include <NoesisGUI/Include/NsGui/Grid.h>
#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/TextBlock.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>
#include <NoesisGUI/Include/NsGui/Button.h>
#include <NoesisGUI/Include/NsGui/ComboBox.h>

#include "Math/Math.h"

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

class EscapeMenuGUI : public Noesis::UserControl
{
public:
	EscapeMenuGUI();
	~EscapeMenuGUI();

	void InitGUI();

private:
	// Escape GUI
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonResumeClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonSettingsClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonLeaveClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonExitClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	void SetRenderStagesInactive();
	bool KeyboardCallback(const LambdaEngine::KeyPressedEvent& event);
	bool MouseButtonCallback(const LambdaEngine::MouseButtonClickedEvent& event);

	void OnSettingsClosed(Noesis::BaseComponent* pSender, const Noesis::DependencyPropertyChangedEventArgs& args);

	void ToggleEscapeMenu();

	NS_IMPLEMENT_INLINE_REFLECTION_(EscapeMenuGUI, Noesis::UserControl, "CrazyCanvas.EscapeMenuGUI")
private:

	// EscapeGUI
	bool 			m_ListenToCallbacks		= false;
	Noesis::Button* m_pSetKeyButton			= nullptr;
	LambdaEngine::THashTable<LambdaEngine::String, LambdaEngine::String> m_KeysToSet;
	float32 m_LookSensitivityPercentageToSet = 0.0f;

	// bool			m_RayTracingEnabled		= false;
	bool			m_MeshShadersEnabled	= false;
	bool			m_FullscreenEnabled		= false;

	bool			m_EscapeMenuActive		= false;
	bool			m_MouseEnabled			= false;

	Noesis::Grid* m_pEscapeGrid				= nullptr;

	glm::vec2 m_WindowSize = glm::vec2(1.0f);

	SettingsGUI* m_pSettingsGUI = nullptr;
};
