#pragma once

#include "CameraTrack.h"
#include "Game/State.h"

class BenchmarkState : public LambdaEngine::State
{
public:
	BenchmarkState();
	~BenchmarkState() = default;

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(float dt) override final;

private:
	static void PrintBenchmarkResults();

private:
	std::unique_ptr<LambdaEngine::Camera> m_pCamera;
	CameraTrack m_CameraTrack;
};
