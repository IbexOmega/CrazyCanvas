#pragma once

#include "Game/State.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	class StateManager;
	class ECSCore;
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
	LambdaEngine::Entity m_DirLight;
	LambdaEngine::Entity m_PointLights[100];
};