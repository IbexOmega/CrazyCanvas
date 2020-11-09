#pragma once

#include "GUI/SavedServerGUI.h"
#include "GUI/ServerInfo.h"

#include "Application/API/Events/NetworkEvents.h"

#include "Networking/API/IPAddress.h"
#include "Networking/API/IPEndPoint.h"

struct HostGameDescription
{
	int8 PlayersNumber = -1;
	int8 MapNumber = -1;
};

enum PopUpCode
{
	CONNECT_ERROR,
	CONNECT_ERROR_INVALID,
	JOIN_ERROR,
	JOIN_ERROR_OFFLINE,
	HOST_ERROR,
	OTHER_ERROR,
	HOST_NOTIFICATION,
	JOIN_NOTIFICATION
};

class MultiplayerGUI : public Noesis::Grid
{
	//ObservableCollection<HostGameDescription> Servers;

public:
	MultiplayerGUI(const LambdaEngine::String& xamlFile);
	~MultiplayerGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	
	bool JoinSelectedServer(Noesis::Grid* pGrid);

	bool OnServerResponse(const LambdaEngine::ServerDiscoveredEvent& event);
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

	void FixedTick(LambdaEngine::Timestamp delta);

private:
	void SetRenderStagesActive();

	void ErrorPopUp(PopUpCode errorCode);
	void NotiPopUP(PopUpCode notificationCode);

	void ErrorPopUpClose();
	void NotiPopUpClose();

	bool CheckServerStatus();
	bool CheckServerSettings(const HostGameDescription& serverSettings);

	bool StartUpServer(const std::string& applicationName, const std::string& commandLine);
	void PopulateServerInfo();

	void HandleServerInfo(ServerInfo& serverInfo, int32 clientHostID, bool forceSave = false);

	NS_IMPLEMENT_INLINE_REFLECTION_(MultiplayerGUI, Noesis::Grid)

private:

	bool m_HasHostedServer = false;
	bool m_RayTracingEnabled = false;
	HostGameDescription m_HostGameDesc;
	SavedServerGUI m_ServerList;

	LambdaEngine::TArray<LambdaEngine::String> m_SavedServerList;

	std::unordered_map<LambdaEngine::IPAddress*, ServerInfo, LambdaEngine::IPAddressHasher> m_Servers;
};