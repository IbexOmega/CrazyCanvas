#pragma once
/*#include "Containers/String.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"*/

#include "GUI/SavedServerGUI.h"

struct HostGameDescription
{
	int8 PlayersNumber = -1;
	int8 MapNumber = -1;
};

struct ServerInfo
{
	Noesis::Ptr<Noesis::Grid> ServerGrid;

	std::string Name;
	std::string MapName;
	uint8 Players;
	uint16 Ping;
	LambdaEngine::Timestamp LastUpdate;

	bool operator==(const ServerInfo& other) const
	{
		return Name == other.Name && MapName == other.MapName && Players == other.Players && Ping == other.Ping;
	}

	bool operator!=(const ServerInfo& other) const
	{
		return Name != other.Name || MapName != other.MapName || Players != other.Players || Ping != other.Ping;
	}

	ServerInfo& operator=(const ServerInfo& other)
	{
		if (this != &other)
		{
			Name		= other.Name;
			MapName		= other.MapName;
			Players		= other.Players;
			Ping		= other.Ping;
			LastUpdate	= other.LastUpdate;
		}
		return *this;
	}
};

enum ErrorCode
{
	CONNECT_ERROR,
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

	bool ConnectEvent(Noesis::BaseComponent* source, const char* event, const char* handler) override;
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonRefreshClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	
	void OnButtonErrorClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	
	void OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	bool OnLANServerFound(const LambdaEngine::ServerDiscoveredEvent& event);



private:
	void SetRenderStagesActive();

	void ErrorPopUp(ErrorCode errorCode);
	void ErrorPopUpClose();

	bool CheckServerSettings(const HostGameDescription& serverSettings);

	void PopulateServerInfo();

	NS_IMPLEMENT_INLINE_REFLECTION_(LobbyGUI, Noesis::Grid)

private:

	bool m_RayTracingEnabled = false;
	HostGameDescription m_HostGameDesc;
	SavedServerGUI m_ServerList;

	std::unordered_map<uint64, ServerInfo> m_Servers;
};