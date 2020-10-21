#pragma once
#include "ECS/System.h"

class HealthSystem : public LambdaEngine::System
{
public:
	bool Init();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final;

	static HealthSystem& GetInstance();

private:
	HealthSystem();
	~HealthSystem();

private:
	LambdaEngine::IDVector m_HealthEntities;
};