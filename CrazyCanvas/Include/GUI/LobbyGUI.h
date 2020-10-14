#pragma once
#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"

#include "Application/API/Events/NetworkEvents.h"

class LobbyGUI : public Noesis::Grid
{
public:
	LobbyGUI(const LambdaEngine::String& xamlFile);
	~LobbyGUI();

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool OnServerFound(const LambdaEngine::ServerDiscoveredEvent& event);

private:
	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid)
};