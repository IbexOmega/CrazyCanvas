#pragma once

#include "Multiplayer/MultiplayerBase.h"

class MultiplayerServer : public MultiplayerBase
{
public:
	MultiplayerServer();
	~MultiplayerServer();

	void Init() override final;
	void TickMainThread(LambdaEngine::Timestamp deltaTime) override final;
	void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) override final;
};