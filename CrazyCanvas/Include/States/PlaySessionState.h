#pragma once

#include "Game/State.h"

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState() = default;
	~PlaySessionState() = default;

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;


};
