#pragma once

#include "FlagSystemBase.h"

class ClientFlagSystem : public FlagSystemBase
{
public:
	ClientFlagSystem();
	~ClientFlagSystem();

	virtual void OnFlagPickedUp(LambdaEngine::Entity playerEntity, LambdaEngine::Entity flagEntity) override final;
	virtual void OnFlagDropped() override final;

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
};