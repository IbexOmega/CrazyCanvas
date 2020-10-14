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




/*struct ServerData : public Noesis::BaseComponent
{
    Noesis::String m_ServerName;
    Noesis::String m_ServerMap;
    Noesis::String m_ServerPing;

    bool m_ServerIsOnline;

    ServerData(const char* n, const char* c, const char* p, bool isOnline) :
        m_ServerName(n), m_ServerMap(c), m_ServerPing(p), m_ServerIsOnline(isOnline)
    {
    };

    NS_IMPLEMENT_INLINE_REFLECTION(ServerData, Noesis::BaseComponent)
    {
        //NsMeta<< Noesis::UIElementData > (Noesis::TypeOf<SelfClass >>("ServerData");
        NsProp("Name", &ServerData::m_ServerName);
        NsProp("Map", &ServerData::m_ServerMap);
        NsProp("Ping", &ServerData::m_ServerPing);
        NsProp("Online", &ServerData::m_ServerIsOnline);
    };
};

class ListViewTest : public Noesis::Grid
{
public:
    ListViewTest() : mGameCollection(*new Noesis::ObservableCollection<ServerData>())
    {
        Noesis::Ptr<ServerData> gameData = *new ServerData("World Of Warcraft", "Blizzard", "Blizzard", true);
        mGameCollection->Add(gameData.GetPtr());
        gameData = *new ServerData("Halo", "Bungie", "Microsoft", true);
        mGameCollection->Add(gameData.GetPtr());
        gameData = *new ServerData("Gears Of War", "Epic", "Microsoft", true);
        mGameCollection->Add(gameData.GetPtr());
    }

    Noesis::ObservableCollection<ServerData>* GetGameCollection() const
    {
        return mGameCollection;
    }

private:
    void AddRow_Click(Noesis::BaseComponent* sender, const RoutedEventArgs& e)
    {
        Ptr<ServerData> gameData = *new ServerData("A New Game", "A New Creator", "A New Publisher", true);
        mGameCollection->Add(gameData.GetPtr());
    }

private:
    Noesis::Ptr<Noesis::ObservableCollection<ServerData>> mGameCollection;

    NS_IMPLEMENT_INLINE_REFLECTION(ListViewTest, Noesis::Grid)
    {
        //NsMeta<Noesis::UIElementData>(Noesis::TypeOf<SelfClass>"ListViewTest");
        NsProp("GameCollection", &ListViewTest::GetGameCollection);
        //NsFunc("AddRow_Click", &ListViewTest::AddRow_Click);
    }
};*/

class SavedServerGUI
{
public:
	SavedServerGUI(const LambdaEngine::String& xamlFile);
	~SavedServerGUI();

	void AddSavedServerItem(Noesis::Grid* pParentGrid, const char* pServerN, const char* pMapN, const char* pPing, bool isRunning);
	void AddLocalServerItem(Noesis::Grid* pParentGrid, const char* pServerN, const char* pMapN, const char* pPing, bool isRunning);
	
    Noesis::Ptr<Noesis::Grid> AddServerItem(Noesis::Grid* pParentGrid, const char* pServerN, const char* pMapN, const char* pPing, bool isRunning);
	
    void UpdateServerItems(const char* pServerN, const char* pMapN, const char* pPing, bool isRunning, bool isLocal);

    void Init(Noesis::ListBox* pListView, Noesis::ListBox* pLocalListView);

    
private:
	uint8 m_ItemCount;

    Noesis::Ptr<Noesis::ListBox> m_SavedServerList;
    Noesis::Ptr<Noesis::ListBox> m_LocalServerList;
};
