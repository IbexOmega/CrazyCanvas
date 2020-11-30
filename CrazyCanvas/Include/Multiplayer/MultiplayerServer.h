#pragma once
#include "Multiplayer/MultiplayerBase.h"

#include "ECS/Systems/Match/ServerFlagSystem.h"
#include "ECS/Systems/Match/ServerShowerSystem.h"
#include "ECS/Systems/Player/WeaponSystem.h"

#include "World/Player/Server/PlayerRemoteSystem.h"

class MultiplayerServer : public MultiplayerBase
{
public:
	MultiplayerServer();
	~MultiplayerServer();

protected:
	void Init() override final;
	void TickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void PostFixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;

private:
	ServerFlagSystem* m_pFlagSystem = nullptr;
	ServerShowerSystem* m_pShowerSystem = nullptr;
	PlayerRemoteSystem m_PlayerRemoteSystem;
};