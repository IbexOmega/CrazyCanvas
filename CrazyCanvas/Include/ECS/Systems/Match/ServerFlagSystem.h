#pragma once

#include "FlagSystemBase.h"

class ServerFlagSystem : public FlagSystemBase
{
public:
	ServerFlagSystem();
	~ServerFlagSystem();

	virtual void OnPlayerFlagCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
};