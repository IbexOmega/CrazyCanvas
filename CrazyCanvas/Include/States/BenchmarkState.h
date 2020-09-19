#pragma once

#include "Game/State.h"
#include "ECS/Entity.h"

class BenchmarkState : public LambdaEngine::State
{
public:
	BenchmarkState();
	~BenchmarkState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(float dt) override final;

private:
	static void PrintBenchmarkResults();

private:
	LambdaEngine::Entity m_Camera;
};
