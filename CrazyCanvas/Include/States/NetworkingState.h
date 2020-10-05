#pragma once

#include "Game/State.h"

class NetworkingState : public LambdaEngine::State
{
public:
	NetworkingState() = default;
	~NetworkingState() = default;

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;
};
