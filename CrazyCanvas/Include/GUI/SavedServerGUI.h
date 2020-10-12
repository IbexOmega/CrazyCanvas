#pragma once
#include "Containers/String.h"

#include "LambdaEngine.h"

#include "NsGui/UserControl.h"
#include "NsGui/Grid.h"
#include "NsGui/GroupBox.h"
#include "NsGui/Slider.h"
#include "NsGui/ListBox.h"
#include "NsGui/Collection.h"

#include "NsCore/BaseComponent.h"




struct ServerData : public Noesis::BaseComponent
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
        NsMeta<Noesis::TypeId>("ServerData");
        NsProp("Name", &ServerData::m_ServerName);
        NsProp("Map", &ServerData::m_ServerMap);
        NsProp("Ping", &ServerData::m_ServerPing);
        NsProp("Online", &ServerData::m_ServerIsOnline);
    };
};

class ListViewTest : public Noesis::Grid
{
public:
    ListViewTest() : mGameCollection(*new Noesis::Collection<ServerData>())
    {
        Ptr<ServerData> gameData = *new ServerData("World Of Warcraft", "Blizzard", "Blizzard", true);
        mGameCollection->Add(gameData.GetPtr());
        gameData = *new ServerData("Halo", "Bungie", "Microsoft", true);
        mGameCollection->Add(gameData.GetPtr());
        gameData = *new ServerData("Gears Of War", "Epic", "Microsoft", true);
        mGameCollection->Add(gameData.GetPtr());
    }

    Noesis::Collection<ServerData>* GetGameCollection() const
    {
        return mGameCollection.GetPtr();
    }

private:
    /*void AddRow_Click(Noesis::BaseComponent* sender, const RoutedEventArgs& e)
    {
        Ptr<ServerData> gameData = *new ServerData("A New Game", "A New Creator", "A New Publisher", true);
        mGameCollection->Add(gameData.GetPtr());
    }*/

private:
    Ptr<Noesis::Collection<ServerData>> mGameCollection;

    NS_IMPLEMENT_INLINE_REFLECTION(ListViewTest, Noesis::Grid)
    {
        NsMeta<Noesis::TypeId>("ListViewTest");
        NsProp("GameCollection", &ListViewTest::GetGameCollection);
        //NsFunc("AddRow_Click", &ListViewTest::AddRow_Click);
    }
};

class SavedServerGUI : public Noesis::Grid
{
	//ObservableCollection<ListBox> Servers;
public:
	SavedServerGUI(const LambdaEngine::String& xamlFile);
	~SavedServerGUI();

	Noesis::ListBox* GetList();

	void AddServerItem(Grid* pParentGrid, uint8 nrOfColumns, const char* serverName, const char* mapName, bool isRunning);


private:


	NS_IMPLEMENT_INLINE_REFLECTION_(SavedServerGUI, Noesis::Grid)

private:
	uint8 m_ItemCount;

	Noesis::ListBox* m_pSavedServerList;
};
