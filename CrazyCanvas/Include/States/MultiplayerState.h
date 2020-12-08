#pragma once
#include "Game/State.h"

#include "Containers/THashTable.h"

#include "Networking/API/IPEndPoint.h"

#include "Events/ServerEvents.h"
#include "Application/API/Events/NetworkEvents.h"

#include "GUI/MultiplayerGUI.h"

#include "GUI/Core/GUIApplication.h"
#include "NoesisPCH.h"

class MultiplayerState : public LambdaEngine::State
{
	friend class MultiplayerGUI;

public:
	MultiplayerState();
	~MultiplayerState();

protected:
	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override;

private:
	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);
	bool OnClientDisconnected(const LambdaEngine::ClientDisconnectedEvent& event);
	bool OnServerOnlineEvent(const ServerOnlineEvent& event);
	bool OnServerOfflineEvent(const ServerOfflineEvent& event);
	bool OnServerUpdatedEvent(const ServerUpdatedEvent& event);

	bool ConnectToServer(const LambdaEngine::IPEndPoint& endPoint, bool isManual);

	bool HasHostedServer() const;
	void StartUpServer();

private:
	Noesis::Ptr<MultiplayerGUI> m_MultiplayerGUI;
	Noesis::Ptr<Noesis::IView> m_View;
	int32 m_ClientHostID;
	bool m_IsManualConnection;
};