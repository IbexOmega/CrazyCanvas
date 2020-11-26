#pragma once

#include "ShowerSystemBase.h"

class ServerShowerSystem : public ShowerSystemBase
{
public:
	ServerShowerSystem();
	~ServerShowerSystem();

	virtual void OnPlayerShowerCollision(LambdaEngine::Entity entity0, LambdaEngine::Entity entity1) override final;

protected:
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
	virtual void FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime) override final;
};