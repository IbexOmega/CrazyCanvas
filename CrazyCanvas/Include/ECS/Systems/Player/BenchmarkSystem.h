#pragma once
#include "ECS/System.h"

class WeaponSystem;

class BenchmarkSystem : public LambdaEngine::System
{
public:
	BenchmarkSystem() = default;
	~BenchmarkSystem() = default;

	void Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	LambdaEngine::IDVector m_WeaponEntities;
	LambdaEngine::IDVector m_LocalPlayerEntities;
};
