#pragma once

#include "FlagSystemBase.h"

class ClientFlagSystem : public FlagSystemBase
{
public:
	ClientFlagSystem();
	~ClientFlagSystem();

	virtual void OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity) override final;
	virtual void OnFlagDropped(LambdaEngine::Entity flagEntity, const glm::vec3& dropPosition) override final;

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;
	virtual void OnDeliveryPointFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

	virtual void InternalAddAdditionalRequiredFlagComponents(LambdaEngine::TArray<LambdaEngine::ComponentAccess>& componentAccesses) override final;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
};