#pragma once

#include "ECS/System.h"
#include "Application\API\Events\NetworkEvents.h"

class PacketDecoderSystem : public LambdaEngine::System
{
public:
	PacketDecoderSystem();
	~PacketDecoderSystem();

	void Init();

private:
	bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);
	virtual void Tick(LambdaEngine::Timestamp deltaTime) override final {};

private:
	LambdaEngine::IDVector m_PlayerEntities;
};