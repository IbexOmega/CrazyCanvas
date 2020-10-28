#pragma once

#include "ECS/System.h"

#include "Game/ECS/Systems/Physics/PhysicsSystem.h"

class FlagSystemBase : public LambdaEngine::System
{
public:
	FlagSystemBase() = default;
	~FlagSystemBase();

	// Remember to call this if overriding FlagSystemBase::Init
	virtual bool Init();

	void FixedTick(LambdaEngine::Timestamp deltaTime);
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override;

	virtual void OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity) = 0;
	virtual void OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition) = 0;

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) = 0;
	virtual void OnDeliveryPointFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) = 0;

protected:
	virtual void InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses) 
	{
		UNREFERENCED_VARIABLE(componentAccesses);
	}

	virtual void InternalAddAdditionalAccesses(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses) 
	{
		UNREFERENCED_VARIABLE(componentAccesses);
	}

	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) = 0;

public:
	FORCEINLINE static FlagSystemBase* GetInstance() 
	{ 
		return s_Instance; 
	}

protected:
	static void CalculateAttachedFlagPosition(
		glm::vec3& flagPosition, 
		glm::quat& flagRotation, 
		const glm::vec3& flagOffset, 
		const glm::vec3& parentPosition, 
		const glm::quat parentRotation);

protected:
	LambdaEngine::IDVector m_Flags;

private:
	inline static FlagSystemBase* s_Instance = nullptr;
};