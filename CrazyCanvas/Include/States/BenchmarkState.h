#pragma once

#include "Game/State.h"
#include "ECS/Entity.h"
#include "World/Level.h"

class Level;

class BenchmarkState : public LambdaEngine::State
{
public:
	BenchmarkState() = default;
	~BenchmarkState();

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(LambdaEngine::Timestamp delta) override final;

private:
	static void PrintBenchmarkResults();

private:
	LambdaEngine::Entity m_Camera;
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[3];
	Level* m_pLevel = nullptr;
};
