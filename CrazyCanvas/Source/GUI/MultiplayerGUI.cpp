#include "Game/State.h"

#include "Engine/EngineConfig.h"
#include "Engine/EngineLoop.h"

#include "Networking/API/NetworkUtils.h"

#include "Game/Multiplayer/Client/ClientSystem.h"
#include "Game/Multiplayer/Server/ServerSystem.h"

#include "Multiplayer/Packet/PacketType.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "States/LobbyState.h"
#include "States/PlaySessionState.h"
#include "States/MultiplayerState.h"

#include "GUI/MultiplayerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#include "NsGui/Rectangle.h"

#include "States/MainMenuState.h"
#include "States/ServerState.h"

#include "Multiplayer/ClientHelper.h"

#include "Application/API/Events/EventQueue.h"

#include "Lobby/PlayerManagerClient.h"
#include "Lobby/ServerManager.h"

#include <string>
#include <sstream>
#include <wchar.h>
#include <windows.h>
#include <Lmcons.h>

#define SERVER_ITEM_COLUMNS 5

using namespace LambdaEngine;
using namespace Noesis;

MultiplayerGUI::MultiplayerGUI(MultiplayerState* pMultiplayerState) :
	m_pMulitplayerState(pMultiplayerState)
{
	Noesis::GUI::LoadComponent(this, "Multiplayer.xaml");

	m_pGridServers			= FrameworkElement::FindName<Grid>("FIND_SERVER_CONTAINER");
	m_pListBoxServersLAN	= FrameworkElement::FindName<ListBox>("LOCAL_SERVER_LIST");
	m_pListBoxServersSaved	= FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST");
	m_pTextBoxAddress		= FrameworkElement::FindName<TextBox>("IP_ADDRESS");
	m_pTextBoxName			= FrameworkElement::FindName<TextBox>("IN_GAME_NAME");

	ErrorPopUpClose();
	NotiPopUpClose();

	// Use Host name as default In Game name
	const Player* pLocalPlayer = PlayerManagerClient::GetPlayerLocal();
	LambdaEngine::String nameStr;
	if (pLocalPlayer)
	{
		nameStr = pLocalPlayer->GetName();
	}
	else
	{
		char name[UNLEN + 1];
		DWORD length = UNLEN + 1;
		GetUserNameA(name, &length);
		nameStr = name;
	}

	TextBox* pNameTextBox = FrameworkElement::FindName<TextBox>("IN_GAME_NAME");
	pNameTextBox->SetMaxLength(MAX_NAME_LENGTH - 1);
	pNameTextBox->SetMaxLines(1);
	pNameTextBox->SetText(nameStr.c_str());
	pNameTextBox->SetText(nameStr.substr(0, glm::min<int32>((int32)nameStr.length(), pNameTextBox->GetMaxLength())).c_str());
}

MultiplayerGUI::~MultiplayerGUI()
{

}

void MultiplayerGUI::AddServerLAN(const ServerInfo& serverInfo)
{
	Ptr<Grid> grid = CreateServerItem(serverInfo);

	m_pGridServers->SetColumn(grid, 1);
	m_pGridServers->SetColumnSpan(grid, 2);
	m_pGridServers->SetRow(grid, 4);

	m_pListBoxServersLAN->GetItems()->Add(grid);
}

void MultiplayerGUI::AddServerSaved(const ServerInfo& serverInfo)
{
	Ptr<Grid> grid = CreateServerItem(serverInfo);

	m_pGridServers->SetColumn(grid, 1);
	m_pGridServers->SetColumnSpan(grid, 2);
	m_pGridServers->SetRow(grid, 4);

	m_pListBoxServersSaved->GetItems()->Add(grid);
}

void MultiplayerGUI::RemoveServerLAN(const ServerInfo& serverInfo)
{
	Grid* pGrid = DeleteServerItem(serverInfo);
	m_pListBoxServersLAN->GetItems()->Remove(pGrid);
}

void MultiplayerGUI::RemoveServerSaved(const ServerInfo& serverInfo)
{
	Grid* pGrid = DeleteServerItem(serverInfo);
	m_pListBoxServersSaved->GetItems()->Remove(pGrid);
}

void MultiplayerGUI::UpdateServerLAN(const ServerInfo& serverInfo)
{
	LambdaEngine::String name;
	ServerInfoToUniqeString(serverInfo, name);
	Grid* pGrid = FrameworkElement::FindName<Grid>(name.c_str());
	if (pGrid)
	{
		UpdateServerItem(pGrid, serverInfo);
	}
}

void MultiplayerGUI::UpdateServerSaved(const ServerInfo& serverInfo)
{
	LambdaEngine::String name;
	ServerInfoToUniqeString(serverInfo, name);
	Grid* pGrid = FrameworkElement::FindName<Grid>(name.c_str());
	if (pGrid)
	{
		UpdateServerItem(pGrid, serverInfo);
	}
}

Ptr<Grid> MultiplayerGUI::CreateServerItem(const ServerInfo& serverInfo)
{
	Ptr<Grid> pGrid = *new Grid();

	for (int i = 0; i < SERVER_ITEM_COLUMNS; i++)
	{
		Ptr<ColumnDefinition> columnDef = *new ColumnDefinition();
		GridLength gridLength = GridLength(20, GridUnitType_Star);
		columnDef->SetWidth(gridLength);
		pGrid->GetColumnDefinitions()->Add(columnDef);
	}

	Ptr<TextBlock> serverName = *new TextBlock();
	Ptr<TextBlock> mapName = *new TextBlock();
	Ptr<TextBlock> players = *new TextBlock();
	Ptr<TextBlock> ping = *new TextBlock();
	Ptr<Noesis::Rectangle> isOnline = *new Noesis::Rectangle();
	Ptr<SolidColorBrush> brush = *new SolidColorBrush();

	isOnline->SetFill(brush);

	pGrid->GetChildren()->Add(serverName);
	pGrid->GetChildren()->Add(mapName);
	pGrid->GetChildren()->Add(players);
	pGrid->GetChildren()->Add(ping);
	pGrid->GetChildren()->Add(isOnline);

	pGrid->SetColumn(serverName, 0);
	pGrid->SetColumn(mapName, 1);
	pGrid->SetColumn(players, 2);
	pGrid->SetColumn(ping, 3);
	pGrid->SetColumn(isOnline, 4);

	UpdateServerItem(pGrid, serverInfo);

	LambdaEngine::String name;
	ServerInfoToUniqeString(serverInfo, name);
	FrameworkElement::GetView()->GetContent()->RegisterName(name.c_str(), pGrid);

	return pGrid;
}

Grid* MultiplayerGUI::DeleteServerItem(const ServerInfo& serverInfo)
{
	LambdaEngine::String name;
	ServerInfoToUniqeString(serverInfo, name);
	Grid* pGrid = FrameworkElement::FindName<Grid>(name.c_str());
	FrameworkElement::GetView()->GetContent()->UnregisterName(name.c_str());
	return pGrid;
}

void MultiplayerGUI::UpdateServerItem(Grid* pGrid, const ServerInfo& serverInfo)
{
	TextBlock* pName				= (TextBlock*)pGrid->GetChildren()->Get(0);
	TextBlock* pMapName				= (TextBlock*)pGrid->GetChildren()->Get(1);
	TextBlock* pPlayerCount			= (TextBlock*)pGrid->GetChildren()->Get(2);
	TextBlock* pPing				= (TextBlock*)pGrid->GetChildren()->Get(3);
	Noesis::Rectangle* pIsOnline	= (Noesis::Rectangle*)pGrid->GetChildren()->Get(4);

	SolidColorBrush* pBrush = (SolidColorBrush*)pIsOnline->GetFill();

	pName->SetText(serverInfo.Name.c_str());
	pMapName->SetText(serverInfo.IsOnline ? serverInfo.MapName.c_str() : "-");
	pPing->SetText((serverInfo.IsOnline ? std::to_string(serverInfo.Ping) + " ms" : "-").c_str());
	pPlayerCount->SetText(serverInfo.IsOnline ? (std::to_string(serverInfo.Players) + "/" + std::to_string(serverInfo.MaxPlayers)).c_str() : "-");
	pBrush->SetColor(serverInfo.IsOnline ? Color::Green() : Color::Red());
}

void MultiplayerGUI::ServerInfoToUniqeString(const ServerInfo& serverInfo, LambdaEngine::String& str) const
{
	str = std::to_string(serverInfo.ServerUID) + std::to_string(serverInfo.IsLAN);
}

const ServerInfo* MultiplayerGUI::GetServerInfoFromGrid(const THashTable<uint64, ServerInfo>& servers, Grid* pGrid) const
{
	for (auto& server : servers)
	{
		const ServerInfo& serverInfo = server.second;
		LambdaEngine::String name;
		ServerInfoToUniqeString(serverInfo, name);
		Grid* pGridServer = FrameworkElement::FindName<Grid>(name.c_str());

		if (pGridServer == pGrid)
			return &serverInfo;
	}
	return nullptr;
}

const char* MultiplayerGUI::GetPlayerName() const
{
	return m_pTextBoxName->GetText();
}

bool MultiplayerGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	NS_CONNECT_EVENT_DEF(pSource, pEvent, pHandler);

	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonBackClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonConnectClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonErrorOKClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonHostGameClick);
	NS_CONNECT_EVENT(Noesis::Button, Click, OnButtonJoinClick);
	return false;
}

void MultiplayerGUI::OnButtonBackClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	State* pMainMenuState = DBG_NEW MainMenuState();
	StateManager::GetInstance()->EnqueueStateTransition(pMainMenuState, STATE_TRANSITION::POP_AND_PUSH);
}

void MultiplayerGUI::OnButtonConnectClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	LambdaEngine::String address	= m_pTextBoxAddress->GetText();
	bool isEndpointValid			= false;
	uint16 defaultPort				= (uint16)EngineConfig::GetUint32Property(EConfigOption::CONFIG_OPTION_NETWORK_PORT);
	IPEndPoint endPoint				= IPEndPoint::Parse(address, defaultPort, &isEndpointValid);

	if (isEndpointValid)
	{
		m_pMulitplayerState->ConnectToServer(endPoint, true);
	}
	else
	{
		ErrorPopUp(CONNECT_ERROR_INVALID);
	}
}

void MultiplayerGUI::OnButtonErrorOKClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	ErrorPopUpClose();
}

void MultiplayerGUI::OnButtonHostGameClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	m_pMulitplayerState->StartUpServer();
}

void MultiplayerGUI::OnButtonJoinClick(Noesis::BaseComponent* pSender, const Noesis::RoutedEventArgs& args)
{
	UNREFERENCED_VARIABLE(pSender);
	UNREFERENCED_VARIABLE(args);

	TabItem* pTab = FrameworkElement::FindName<TabItem>("LOCAL");
	ListBox* pBox = pTab->GetIsSelected() ? m_pListBoxServersLAN : m_pListBoxServersSaved;
	Grid* pSelectedItem = (Grid*)pBox->GetSelectedItem();

	if (pSelectedItem)
	{
		const THashTable<uint64, ServerInfo>& serversLAN = ServerManager::GetServersLAN();
		const ServerInfo* pServerInfo = GetServerInfoFromGrid(serversLAN, pSelectedItem);

		bool result = false;

		if (pServerInfo)
		{
			result = m_pMulitplayerState->ConnectToServer(pServerInfo->EndPoint, false);
		}
		else
		{
			const THashTable<uint64, ServerInfo>& serversWAN = ServerManager::GetServersWAN();
			pServerInfo = GetServerInfoFromGrid(serversWAN, pSelectedItem);

			if (pServerInfo)
			{
				result = m_pMulitplayerState->ConnectToServer(pServerInfo->EndPoint, false);
			}
		}

		if (result)
			NotiPopUP(JOIN_NOTIFICATION);
		else
			ErrorPopUp(JOIN_ERROR_OFFLINE);
	}
	else
	{
		ErrorPopUp(JOIN_ERROR);
	}
}

void MultiplayerGUI::ErrorPopUp(PopUpCode errorCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("ERROR_BOX_TEXT");

	switch (errorCode)
	{
	case CONNECT_ERROR:				pTextBox->SetText("Couldn't Connect To Server!");		break;
	case CONNECT_ERROR_INVALID:		pTextBox->SetText("The address format is invalid!");	break;
	case JOIN_ERROR:				pTextBox->SetText("No Server Selected!");				break;
	case JOIN_ERROR_OFFLINE:		pTextBox->SetText("Server is offline!");				break;
	case HOST_ERROR:				pTextBox->SetText("Couldn't Host Server!");				break;
	case OTHER_ERROR:				pTextBox->SetText("Something Went Wrong!");				break;
	}

	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void MultiplayerGUI::NotiPopUP(PopUpCode notificationCode)
{
	TextBlock* pTextBox = FrameworkElement::FindName<TextBlock>("NOTIFICATION_BOX_TEXT");

	switch (notificationCode)
	{
	case HOST_NOTIFICATION:	pTextBox->SetText("Server Starting...");	break;
	case JOIN_NOTIFICATION:	pTextBox->SetText("Joining Server...");		break;
	}

	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Visible);
}

void MultiplayerGUI::ErrorPopUpClose()
{
	FrameworkElement::FindName<Grid>("ERROR_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

void MultiplayerGUI::NotiPopUpClose()
{
	FrameworkElement::FindName<Grid>("NOTIFICATION_BOX_CONTAINER")->SetVisibility(Visibility_Hidden);
}

bool MultiplayerGUI::CheckServerSettings(const HostGameDescription& serverSettings)
{
	if (serverSettings.PlayersNumber == -1)
		return false;
	else if (serverSettings.MapNumber == -1)
		return false;

	return true;
}