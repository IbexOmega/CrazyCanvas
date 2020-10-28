#pragma once

#include "Multiplayer/MultiplayerBase.h"
#include "World/Player/Client/PlayerLocalSystem.h"
#include "World/Player/Client/PlayerForeignSystem.h"
#include "ECS/Systems/Match/ClientFlagSystem.h"
#include "ECS/Systems/Multiplayer/Client/NetworkPositionSystem.h"

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
	PlayerLocalSystem m_PlayerLocal;
	PlayerForeignSystem m_PlayerForeignSystem;
	NetworkPositionSystem m_NetworkPositionSystem;

	ClientFlagSystem* m_pFlagSystem = nullptr;
};