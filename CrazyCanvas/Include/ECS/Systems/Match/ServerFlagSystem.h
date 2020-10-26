#pragma once

#include "FlagSystemBase.h"

class ServerFlagSystem : public FlagSystemBase
{
public:
	ServerFlagSystem();
	~ServerFlagSystem();

	virtual void OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity) override final;
	virtual void OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition) override final;

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;
	virtual void OnBaseFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

protected:
	virtual void InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses) override final;
	virtual void InternalAddAdditionalAccesses(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses) override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
};