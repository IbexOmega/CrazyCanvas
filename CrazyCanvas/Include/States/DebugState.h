#pragma once

#include "Game/State.h"

class DebugState : public LambdaEngine::State
{
public:
	DebugState();
	DebugState(LambdaEngine::State* pOther);
	~DebugState();

	void Init() override final;

	void Resume() override final;
	void Pause() override final;

	void Tick(float dt) override final;
};