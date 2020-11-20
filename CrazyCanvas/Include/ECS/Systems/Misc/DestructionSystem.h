#pragma once

#include "ECS/System.h"

class DestructionSystem : public LambdaEngine::System
{
public:
	DestructionSystem();
	~DestructionSystem();

	bool Init();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	LambdaEngine::IDVector m_DestructionEntities;
};