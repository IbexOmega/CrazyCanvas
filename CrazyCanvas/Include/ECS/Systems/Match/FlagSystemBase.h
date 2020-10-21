#pragma once

#include "ECS/System.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

class FlagSystemBase : public LambdaEngine::System
{
public:
	FlagSystemBase() = default;
	~FlagSystemBase();

	virtual bool Init();

	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;

	virtual void OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity) = 0;
	virtual void OnFlagDropped() = 0;

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) = 0;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;

public:
	FORCEINLINE static FlagSystemBase* GetInstance() 
	{ 
		return s_Instance; 
	}

protected:
	LambdaEngine::IDVector m_Flags;

private:
	inline static FlagSystemBase* s_Instance = nullptr;
};