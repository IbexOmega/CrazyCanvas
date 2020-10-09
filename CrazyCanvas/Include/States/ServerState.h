#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

#include "Application/API/Events/NetworkEvents.h"

class Level;

class ServerState :
	public LambdaEngine::State,
	public LambdaEngine::INetworkDiscoveryServer
{
public:
	ServerState() = default;
	~ServerState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	bool OnClientConnected(const LambdaEngine::ClientConnectedEvent& event);

	virtual void Tick(LambdaEngine::Timestamp delta) override;

	virtual void OnNetworkDiscoveryPreTransmit(const LambdaEngine::BinaryEncoder& encoder) override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	Level* m_pLevel = nullptr;
};
