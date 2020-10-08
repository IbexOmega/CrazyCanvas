#pragma once

#include "Game/State.h"

#include "Application/API/Events/KeyEvents.h"

#include "Networking/API/UDP/INetworkDiscoveryServer.h"

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

	virtual void Tick(LambdaEngine::Timestamp delta) override;

	virtual void OnNetworkDiscoveryPreTransmit(const LambdaEngine::BinaryEncoder& encoder) override;

	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);
};
