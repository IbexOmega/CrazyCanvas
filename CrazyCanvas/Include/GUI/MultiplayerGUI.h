#pragma once

#include "Lobby/ServerInfo.h"

#include "Containers/THashTable.h"

#include "NsCore/Ptr.h"
#include "NsGui/Grid.h"
#include "NsGui/ListBox.h"
#include "NsGui/TextBox.h"

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

class MultiplayerState;

class MultiplayerGUI : public Noesis::Grid
{
public:
	MultiplayerGUI(MultiplayerState* pMulitplayerState);
	~MultiplayerGUI();

	void AddServerLAN(const ServerInfo& serverInfo);
	void AddServerSaved(const ServerInfo& serverInfo);
	void RemoveServerLAN(const ServerInfo& serverInfo);
	void RemoveServerSaved(const ServerInfo& serverInfo);
	void UpdateServerLAN(const ServerInfo& serverInfo);
	void UpdateServerSaved(const ServerInfo& serverInfo);

	const char* GetPlayerName() const;

private:
	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;
	void OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);
	void OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args);

	Noesis::Grid* CreateServerItem(const ServerInfo& serverInfo);
	Noesis::Grid* DeleteServerItem(const ServerInfo& serverInfo);
	void UpdateServerItem(Noesis::Grid* pGrid, const ServerInfo& serverInfo);
	void ServerInfoToUniqeString(const ServerInfo& serverInfo, LambdaEngine::String& str) const;
	const ServerInfo* GetServerInfoFromGrid(const LambdaEngine::THashTable<uint64, ServerInfo>& servers, Noesis::Grid* pGrid) const;

private:
	Noesis::Grid* m_pGridServers;
	Noesis::ListBox* m_pListBoxServersLAN;
	Noesis::ListBox* m_pListBoxServersSaved;
	Noesis::TextBox* m_pTextBoxAddress;
	Noesis::TextBox* m_pTextBoxName;

	MultiplayerState* m_pMulitplayerState;

private:
	void ErrorPopUp(PopUpCode errorCode);
	void NotiPopUP(PopUpCode notificationCode);

	void ErrorPopUpClose();
	void NotiPopUpClose();

	bool CheckServerSettings(const HostGameDescription& serverSettings);

	NS_IMPLEMENT_INLINE_REFLECTION_(MultiplayerGUI, Noesis::Grid)
};