#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

class Level;
class FlagSystemBase;

class ServerState :
	public LambdaEngine::State
{
public:
	ServerState() = default;
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

	virtual void Tick(LambdaEngine::Timestamp delta) override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

	bool OnServerDiscoveryPreTransmit(const LambdaEngine::ServerDiscoveryPreTransmitEvent& event);

private:
	Level* m_pLevel = nullptr;
	std::string m_ServerName;

	FlagSystemBase* m_pFlagSystem = nullptr;
};
