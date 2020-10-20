#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Multiplayer/PacketDecoderSystem.h"

class MultiplayerBase
{
public:
	DECL_ABSTRACT_CLASS_NO_DEFAULT(MultiplayerBase);

	void InitInternal();
	void TickMainThreadInternal(LambdaEngine::Timestamp deltaTime);
	void FixedTickMainThreadInternal(LambdaEngine::Timestamp deltaTime);

protected:
	virtual void Init() = 0;
	virtual void TickMainThread(LambdaEngine::Timestamp deltaTime) = 0;
	virtual void FixedTickMainThread(LambdaEngine::Timestamp deltaTime) = 0;

private:
	PacketDecoderSystem m_PacketDecoderSystem;
};