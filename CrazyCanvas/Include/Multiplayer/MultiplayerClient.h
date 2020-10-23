#pragma once
#include "Multiplayer/MultiplayerBase.h"

#include "World/Player/PlayerLocal.h"
#include "World/Player/PlayerForeignSystem.h"

#include "ECS/Systems/Match/ClientFlagSystem.h"
#include "ECS/Systems/Player/WeaponSystem.h"

class MultiplayerClient : public MultiplayerBase
{
public:
	MultiplayerClient();
	~MultiplayerClient();

protected:
	void Init() override final;
	void TickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;

private:
	PlayerLocal m_PlayerLocal;
	PlayerForeignSystem m_PlayerForeignSystem;
	WeaponSystem m_WeaponSystem;
	ClientFlagSystem* m_pFlagSystem = nullptr;
};