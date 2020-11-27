#pragma once
#include "ECS/System.h"

#include "Time/API/Timestamp.h"

#include "ECS/Components/Multiplayer/PacketComponent.h"

#include "Multiplayer/Packet/PacketPlayerAction.h"

class PlayerRemoteSystem : public LambdaEngine::System
{
public:
	PlayerRemoteSystem();
	~PlayerRemoteSystem();

	void Init();

	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final { UNREFERENCED_VARIABLE(deltaTime); };
	
	void OnEntityRemoved(LambdaEngine::Entity entity);

private:
	LambdaEngine::IDVector m_Entities;
	LambdaEngine::THashTable<LambdaEngine::Entity, PacketPlayerAction> m_CurrentGameStates;
};