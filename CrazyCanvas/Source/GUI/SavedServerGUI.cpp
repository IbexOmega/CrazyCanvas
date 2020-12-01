#pragma once 

#include "GUI/SavedServerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"

#define SERVER_ITEM_COLUMNS 5

using namespace LambdaEngine;
using namespace Noesis;

SavedServerGUI::SavedServerGUI()
{

}

SavedServerGUI::~SavedServerGUI()
{

}

void SavedServerGUI::Init(ListBox* pSavedListView, ListBox* pLocalListView)
{
	m_SavedServerList = *pSavedListView;
	m_LocalServerList = *pLocalListView;
}

void SavedServerGUI::AddServer(Grid* pParentGrid, ServerInfo& serverInfo)
{
	Ptr<Grid> grid = AddServerItem(pParentGrid, serverInfo);

	if (serverInfo.IsLAN)
	{
		if (m_LocalServerList->GetItems()->Add(grid) == -1)
		{
			LOG_INFO("Refreshing Local List");
		}
	}
	else
	{
		if (m_SavedServerList->GetItems()->Add(grid) == -1)
		{
			LOG_INFO("Refreshing Saved List");
		}
	}
}

void SavedServerGUI::RemoveServer(ServerInfo& serverInfo)
{
	m_LocalServerList->GetItems()->Remove(serverInfo.GridUI);
	m_SavedServerList->GetItems()->Remove(serverInfo.GridUI);
	serverInfo.GridUI = nullptr;
}

Ptr<Grid> SavedServerGUI::AddServerItem(Grid* pParentGrid, ServerInfo& serverInfo)
{
	Ptr<Grid> grid = *new Grid();

	for (int i = 0; i < SERVER_ITEM_COLUMNS; i++)
	{
		Ptr<ColumnDefinition> columnDef = *new ColumnDefinition();
		GridLength gridLength = GridLength(20, GridUnitType_Star);
		columnDef->SetWidth(gridLength);
		grid->GetColumnDefinitions()->Add(columnDef);
	}

	pParentGrid->SetColumn(grid, 1);
	pParentGrid->SetColumnSpan(grid, 2);
	pParentGrid->SetRow(grid, 4);

	Ptr<TextBlock> serverName	= *new TextBlock();
	Ptr<TextBlock> mapName		= *new TextBlock();
	Ptr<TextBlock> players		= *new TextBlock();
	Ptr<TextBlock> ping			= *new TextBlock();
	Ptr<Rectangle> isOnline		= *new Rectangle();
	Ptr<SolidColorBrush> brush	= *new SolidColorBrush();

	isOnline->SetFill(new SolidColorBrush());

	grid->GetChildren()->Add(serverName);
	grid->GetChildren()->Add(mapName);
	grid->GetChildren()->Add(players);
	grid->GetChildren()->Add(ping);
	grid->GetChildren()->Add(isOnline);

	grid->SetColumn(serverName,	0);
	grid->SetColumn(mapName,	1);
	grid->SetColumn(players,	2);
	grid->SetColumn(ping,		3);
	grid->SetColumn(isOnline,	4);

	serverInfo.GridUI = grid;

	UpdateServerInfo(serverInfo);

	return grid;
}

void SavedServerGUI::UpdateServerInfo(const ServerInfo& serverInfo)
{
	TextBlock* pName		= (TextBlock*)serverInfo.GridUI->GetChildren()->Get(0);
	TextBlock* pMapName		= (TextBlock*)serverInfo.GridUI->GetChildren()->Get(1);
	TextBlock* pPlayerCount	= (TextBlock*)serverInfo.GridUI->GetChildren()->Get(2);
	TextBlock* pPing		= (TextBlock*)serverInfo.GridUI->GetChildren()->Get(3);
	Rectangle* pIsOnline	= (Rectangle*)serverInfo.GridUI->GetChildren()->Get(4);

	SolidColorBrush* pBrush = (SolidColorBrush*)pIsOnline->GetFill();

	pName->SetText(serverInfo.Name.c_str());
	pMapName->SetText(serverInfo.IsOnline ? serverInfo.MapName.c_str() : "-");
	pPing->SetText((serverInfo.IsOnline ? std::to_string(serverInfo.Ping) + " ms" : "-").c_str());
	pPlayerCount->SetText(serverInfo.IsOnline ? (std::to_string(serverInfo.Players) + "/" + std::to_string(serverInfo.MaxPlayers)).c_str() : "-");
	pBrush->SetColor(serverInfo.IsOnline ? Color::Green() : Color::Red());
}