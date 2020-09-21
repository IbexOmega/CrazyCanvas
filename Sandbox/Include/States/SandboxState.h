#pragma once

#include "Game/State.h"

#include "ECS/ECSCore.h"
#include "Containers/TArray.h"

#include "Application/API/Events/KeyEvents.h"

namespace LambdaEngine
{
	class StateManager;
	class Scene;
}

class SandboxState : public LambdaEngine::State
{
public:
	SandboxState();
	SandboxState(LambdaEngine::State* pOther);
	~SandboxState();

	void Init() override final;

	void Resume() override final;
	void Pause() override final;

	void Tick(LambdaEngine::Timestamp delta);

private:
	bool OnKeyPressed(const LambdaEngine::KeyPressedEvent& event);

private:
	LambdaEngine::TArray<LambdaEngine::Entity> m_Entities;

};