#pragma once
#include "Containers/String.h"

#include "LambdaEngine.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/TabItem.h"
#include "NsGui/TextBlock.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"
#include "NsGui/ObservableCollection.h"

#include "NsCore/BaseComponent.h"
#include "NsCore/Type.h"

#include "GUI/ServerInfo.h"

class SavedServerGUI
{
public:
	SavedServerGUI();
	~SavedServerGUI();

	void AddServer(Noesis::Grid* pParentGrid, ServerInfo& serverInfo);
	
	void UpdateServerInfo(const ServerInfo& serverInfo);

	void Init(Noesis::ListBox* pListView, Noesis::ListBox* pLocalListView);

private:
	Noesis::Ptr<Noesis::Grid> AddServerItem(Noesis::Grid* pParentGrid, ServerInfo& serverInfo);

private:
	Noesis::Ptr<Noesis::ListBox> m_SavedServerList;
	Noesis::Ptr<Noesis::ListBox> m_LocalServerList;
};
