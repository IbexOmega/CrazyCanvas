#pragma once

#include "CameraTrack.h"
#include "Game/State.h"

class PlaySessionState : public LambdaEngine::State
{
public:
	PlaySessionState();
	~PlaySessionState() = default;

	void Init() override final;

	void Resume() override final {};
	void Pause() override final {};

	void Tick(float dt) override final;

private:
	std::unique_ptr<LambdaEngine::Camera> m_pCamera;
};
