#pragma once

#include "Multiplayer/MultiplayerBase.h"
#include "World/Player/PlayerLocal.h"
#include "World/Player/PlayerForeignSystem.h"

class MultiplayerClient : public MultiplayerBase
{
public:
	MultiplayerClient();
	~MultiplayerClient();

	void Init() override final;
	void TickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;

private:
	PlayerLocal m_PlayerLocal;
	PlayerForeignSystem m_PlayerForeignSystem;
};