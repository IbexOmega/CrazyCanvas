#pragma once

#include "Match/MatchBase.h"

#include "Multiplayer/Packet/MultiplayerEvents.h"
#include "Multiplayer/Packet/CreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"

class MatchClient : public MatchBase
{
public:
	MatchClient() = default;
	~MatchClient();

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;

	bool OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<CreateLevelObject>& event);
	bool OnPacketTeamScoredReceived(const PacketReceivedEvent<PacketTeamScored>& event);
};