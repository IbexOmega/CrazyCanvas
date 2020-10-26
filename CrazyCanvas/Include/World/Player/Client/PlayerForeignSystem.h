#pragma once

#include "ECS/System.h"

#include "Time/API/Timestamp.h"

class PlayerForeignSystem : public LambdaEngine::System
{
public:
	PlayerForeignSystem();
	~PlayerForeignSystem();

	void Init();

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };

private:
	LambdaEngine::IDVector m_Entities;
};