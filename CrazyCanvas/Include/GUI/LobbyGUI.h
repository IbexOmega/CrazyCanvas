#pragma once
#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"


class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI(const LambdaEngine::String& xamlFile);
	~LobbyGUI();

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

private:
	void SetRenderStagesActive();
	void ErrorPopUp();
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid)

private:

	bool m_RayTracingEnabled = false;
};