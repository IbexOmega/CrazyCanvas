#pragma once

#include "Game/State.h"

#include "Application/API/Events/NetworkEvents.h"

namespace LambdaEngine
{
	class IClient;
	class NetworkSegment;
}

class Level;

class NetworkingState : public LambdaEngine::State
{
public:
	NetworkingState() = default;
	~NetworkingState();

	void Init() override final;

	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;

private:
	Level* m_pLevel = nullptr;
};
