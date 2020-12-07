#pragma once

#include "ECS/System.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

class ShowerSystemBase : public LambdaEngine::System
{
public:
	ShowerSystemBase() = default;
	~ShowerSystemBase();

	virtual bool Init();

	void FixedTick(LambdaEngine::Timestamp deltaTime);
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;

	virtual void OnPlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) = 0;

	static void PlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1)
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance->OnPlayerShowerCollision(entity0, entity1);
	}

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;
	virtual void FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime) = 0;

public:
	FORCEINLINE static ShowerSystemBase* GetInstance()
	{
		return s_pInstance;
	}

protected:
	LambdaEngine::IDVector m_PlayerEntities;

private:
	inline static ShowerSystemBase* s_pInstance = nullptr;
};