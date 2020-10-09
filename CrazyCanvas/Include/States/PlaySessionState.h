#pragma once

#include "Game/State.h"

class Level;

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState() = default;
	~PlaySessionState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;

private:
	Level* m_pLevel = nullptr;
};
