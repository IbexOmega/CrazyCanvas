#pragma once

#include "ECS/System.h"

class WeaponSystem : public LambdaEngine::System
{
public:
	WeaponSystem() = default;
	~WeaponSystem() = default;

	bool Init();

	void Tick(LambdaEngine::Timestamp deltaTime) override final;

private:
	LambdaEngine::IDVector m_PlayerEntities;
};
