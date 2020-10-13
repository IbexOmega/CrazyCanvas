#pragma once 

#include "GUI/SavedServerGUI.h"
#include "GUI/Core/GUIApplication.h"

#include "NoesisPCH.h"


using namespace LambdaEngine;
using namespace Noesis;


SavedServerGUI::SavedServerGUI(const LambdaEngine::String& xamlFile) :
	m_ItemCount(0)
{

}

void SavedServerGUI::Init(ListBox* pListView)
{
	/*textBlock = *new ObservableCollection<TextBlock>();
	tBlock = *new TextBlock("ChrilleBoi");
	textBlock->Add(tBlock);
	m_pSavedServerList->SetItemsSource(textBlock);*/
	
	m_pSavedServerList = *pListView;

}

SavedServerGUI::~SavedServerGUI()
{
}

void SavedServerGUI::AddServerItem(Grid* pParentGrid, const char* pServerN, const char* pMapN, const char* pPing, bool isRunning)
{
	
	Ptr<Grid> grid = *new Grid();

	for (int i = 0; i < 4; i++)
	{
		Ptr<ColumnDefinition> pColumnDef = *new ColumnDefinition();
		GridLength gridLength = GridLength(25, GridUnitType_Star);
		pColumnDef->SetWidth(gridLength);
		grid->GetColumnDefinitions()->Add(pColumnDef);
	}

	Ptr<RowDefinition> pRowDef = *new RowDefinition();
	GridLength rowLength =GridLength(50);
	pRowDef->SetHeight(rowLength);
	grid->GetRowDefinitions()->Add(pRowDef);

	//m_pSavedServerList->SetItemsSource(Servers);
	//m_pSavedServerList->GetChildren()->Add(m_pSavedServerList);
	//glöm inte sätta row också när du löst detta bajsproblemet. 
	pParentGrid->SetColumn(grid, 1);
	pParentGrid->SetColumnSpan(grid, 2);
	pParentGrid->SetRow(grid, 4);

	Ptr<TextBlock> pServerName	= *new TextBlock();
	Ptr<TextBlock> pMapName		= *new TextBlock();
	Ptr<TextBlock> pping		= *new TextBlock();
	Ptr<Rectangle> pIsRunning	= *new Rectangle();

	pServerName->SetText(pServerN);
	pMapName->SetText(pMapN);
	pping->SetText(pPing);

	Ptr<SolidColorBrush> pBrush = *new SolidColorBrush();
	Color color = Color();

	if (isRunning)
		pBrush->SetColor(color.Green());
	else
		pBrush->SetColor(color.Red());

	pIsRunning->SetFill(pBrush);

	grid->GetChildren()->Add(pServerName);
	grid->GetChildren()->Add(pMapName);
	grid->GetChildren()->Add(pping);
	grid->GetChildren()->Add(pIsRunning);

	grid->SetColumn(pServerName, 0);
	grid->SetColumn(pMapName, 1);
	grid->SetColumn(pping, 2);
	grid->SetColumn(pIsRunning, 3);
	//grid->SetRow(pServerName, m_ItemCount++);


	LOG_MESSAGE(pParentGrid->GetName());
	LOG_MESSAGE(m_pSavedServerList->GetName());

	if (m_pSavedServerList->GetItems()->Add(grid) == -1)
	{
		LOG_ERROR("SKIT ON ME");
	}
}

