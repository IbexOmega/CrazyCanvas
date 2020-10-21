#pragma once
#include "ECS/System.h"

class HealthSystem : public LambdaEngine::System
{
public:
	HealthSystem();
	~HealthSystem();

	void Init();



private:
	LambdaEngine::IDVector m_HealthEntities;
};