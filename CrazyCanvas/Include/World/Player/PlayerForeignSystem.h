#pragma once

#include "ECS/System.h"

#include "Time/API/Timestamp.h"

#include "Containers/TArray.h"

#include "World/Player/PlayerGameState.h"

#include "Application/API/Events/NetworkEvents.h"

class PlayerForeignSystem : public LambdaEngine::System
{
public:
	PlayerForeignSystem();
	~PlayerForeignSystem();

	void Init();

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final 
	{
	}

private:
	LambdaEngine::IDVector m_Entities;
};