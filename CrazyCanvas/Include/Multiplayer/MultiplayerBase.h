#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Multiplayer/PacketDecoderSystem.h"

class MultiplayerBase
{
public:
	DECL_ABSTRACT_CLASS_NO_DEFAULT(MultiplayerBase);

	virtual void Init();
	virtual void TickMainThread(LambdaEngine::Timestamp deltaTime);
	virtual void FixedTickMainThread(LambdaEngine::Timestamp deltaTime);

private:
	PacketDecoderSystem m_PacketDecoderSystem;
};