#pragma once

#include "Game/State.h"
namespace LambdaEngine
{
	class StateManager;
	class ECSCore;
	class Scene;
}

class DebugState : public LambdaEngine::State
{
public:
	DebugState();
	DebugState(LambdaEngine::State* pOther);
	~DebugState();

	void Init();

	void Resume();
	void Pause();

	void Tick(float dt);

};