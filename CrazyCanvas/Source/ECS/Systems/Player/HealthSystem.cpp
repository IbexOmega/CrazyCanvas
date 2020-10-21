#include "ECS/Systems/Player/HealthSystem.h"

HealthSystem::HealthSystem()
	: m_HealthEntities()
{
}

HealthSystem::~HealthSystem()
{
}

bool HealthSystem::Init()
{


	return true;
}

void HealthSystem::Tick(LambdaEngine::Timestamp deltaTime)
{
}

HealthSystem& HealthSystem::GetInstance()
{
	static HealthSystem instance;
	return instance;
}
