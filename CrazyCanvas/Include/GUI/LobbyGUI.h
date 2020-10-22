#pragma once
/*#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"*/

#include "GUI/SavedServerGUI.h"
#include "GUI/ServerInfo.h"

struct HostGameDescription
{
	int8 PlayersNumber = -1;
	int8 MapNumber = -1;
};

enum ErrorCode
{
	CONNECT_ERROR,
	JOIN_ERROR,
	HOST_ERROR,
	OTHER_ERROR
};

#include "Application/API/Events/NetworkEvents.h"

class LobbyGUI : public Noesis::Grid
{
	//ObservableCollection<HostGameDescription> Servers;

public:
	LobbyGUI(const LambdaEngine::String& xamlFile);
	~LobbyGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	
	void StartSelectedServer(Noesis::Grid* pGrid);


	bool OnLANServerFound(const LambdaEngine::ServerDiscoveredEvent& event);

	void FixedTick(LambdaEngine::Timestamp delta);

private:
	void SetRenderStagesActive();

	void ErrorPopUp(ErrorCode errorCode);
	void ErrorPopUpClose();

	bool CheckServerStatus();
	bool CheckServerSettings(const HostGameDescription& serverSettings);

	void HostServer();
	bool StartUpServer(const char* pApplicationName, char* pCommandLine);
	void PopulateServerInfo();

	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid)

private:

	bool m_RayTracingEnabled = false;
	HostGameDescription m_HostGameDesc;
	SavedServerGUI m_ServerList;

	std::unordered_map<uint64, ServerInfo> m_Servers;
};