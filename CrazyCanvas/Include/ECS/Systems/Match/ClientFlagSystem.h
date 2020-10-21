#pragma once

#include "FlagSystemBase.h"

class ClientFlagSystem : public FlagSystemBase
{
public:
	ClientFlagSystem();
	~ClientFlagSystem();

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
};