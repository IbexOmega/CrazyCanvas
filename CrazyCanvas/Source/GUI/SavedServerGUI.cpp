#pragma once 

#include "GUI/SavedServerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"


using namespace LambdaEngine;
using namespace Noesis;

SavedServerGUI::SavedServerGUI(const LambdaEngine::String& xamlFile) :
	m_ItemCount(0)
{
	Noesis::GUI::LoadComponent(this, xamlFile.c_str());
	m_pSavedServerList = FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST");


	//FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST")
}

SavedServerGUI::~SavedServerGUI()
{
}


Noesis::ListBox* SavedServerGUI::GetList()
{
	return m_pSavedServerList;
}

void SavedServerGUI::AddServerItem(Grid* pParentGrid, uint8 nrOfColumns, const char* serverName, const char* mapName, bool isRunning)
{
	//ListBox* pSavedServerList = FrameworkElement::FindName<ListBox>("SAVED_SERVER_LIST");
	Ptr<Grid> grid = *new Grid();

	for (int i = 0; i < nrOfColumns; i++)
	{
		Ptr<ColumnDefinition> columnDef = *new ColumnDefinition();
		const GridLength gridLength = *new GridLength(50);
		columnDef->SetWidth(gridLength);
		grid->GetColumnDefinitions()->Add(columnDef);
	}

	/*Ptr<RowDefinition> rowDef = *new RowDefinition();
	const GridLength rowLength = *new GridLength(50);
	rowDef->SetHeight(rowLength);
	grid->GetRowDefinitions()->Add(rowDef);*/

	ColumnDefinitionCollection* collection = grid->GetColumnDefinitions();

	LOG_MESSAGE("Columns: %d", collection->Count());
	//m_pSavedServerList->SetItemsSource(Servers);
	//m_pSavedServerList->GetChildren()->Add(m_pSavedServerList);
	//glöm inte sätta row också när du löst detta bajsproblemet. 
	pParentGrid->SetColumn(grid, 1);
	pParentGrid->SetColumnSpan(grid, 2);
	pParentGrid->SetRow(grid, 4);

	Ptr<TextBlock> pServerName	= *new TextBlock();
	Ptr<TextBlock> pMapName		= *new TextBlock();
	Ptr<Rectangle> pIsRunning	= *new Rectangle();

	pServerName->SetText(serverName);
	pMapName->SetText(mapName);

	Ptr<SolidColorBrush> pBrush = *new SolidColorBrush();
	const Color color = *new Color();

	if (isRunning)
		pBrush->SetColor(color.Green());
	else
		pBrush->SetColor(color.Red());

	pIsRunning->SetFill(pBrush);

	grid->GetChildren()->Add(pServerName);
	grid->GetChildren()->Add(pMapName);
	grid->GetChildren()->Add(pIsRunning);

	grid->SetColumn(pServerName, 0);
	grid->SetColumn(pMapName, 1);
	grid->SetColumn(pIsRunning, 2);
	//grid->SetRow(pServerName, m_ItemCount++);


	LOG_MESSAGE(pParentGrid->GetName());
	LOG_MESSAGE(m_pSavedServerList->GetName());

	if (m_pSavedServerList->GetItems()->Add(grid) == -1)
	{
		LOG_ERROR("SKIT ON ME");
	}
}
