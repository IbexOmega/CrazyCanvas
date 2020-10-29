#pragma once

#include "ECS/System.h"

class PlayerAnimationSystem : public LambdaEngine::System
{
public:
	PlayerAnimationSystem();
	~PlayerAnimationSystem();

	bool Init();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	LambdaEngine::IDVector m_PlayerEntities;
};