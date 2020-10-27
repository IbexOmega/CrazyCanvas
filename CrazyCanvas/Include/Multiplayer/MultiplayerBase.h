#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "ECS/Systems/Multiplayer/PacketTranscoderSystem.h"

class MultiplayerBase
{
public:
	DECL_UNIQUE_CLASS(MultiplayerBase);

	MultiplayerBase();
	virtual ~MultiplayerBase();

	void InitInternal();
	void TickMainThreadInternal(LambdaEngine::Timestamp deltaTime);
	void FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime);

protected:
	virtual void Init() = 0;
	virtual void TickMainThread(LambdaEngine::Timestamp deltaTime) = 0;
	virtual void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) = 0;

private:
	PacketTranscoderSystem m_PacketDecoderSystem;
};