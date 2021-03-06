#pragma once

#include "Match/MatchBase.h"

#include "Events/PacketEvents.h"
#include "Events/PlayerEvents.h"

#include "Multiplayer/Packet/PacketCreateLevelObject.h"
#include "Multiplayer/Packet/PacketTeamScored.h"
#include "Multiplayer/Packet/PacketDeleteLevelObject.h"
#include "Multiplayer/Packet/PacketGameOver.h"
#include "Multiplayer/Packet/PacketMatchReady.h"
#include "Multiplayer/Packet/PacketMatchStart.h"
#include "Multiplayer/Packet/PacketMatchBegin.h"



class MatchClient : public MatchBase
{
public:
	MatchClient();
	~MatchClient();

	virtual void KillPlaneCallback(LambdaEngine::Entity killPlaneEntity, LambdaEngine::Entity otherEntity) override final;

protected:
	virtual bool InitInternal() override final;
	virtual void TickInternal(LambdaEngine::Timestamp deltaTime) override final;
	virtual bool ResetMatchInternal() override final;

	virtual bool OnWeaponFired(const WeaponFiredEvent& event) override final;

	bool OnPacketCreateLevelObjectReceived(const PacketReceivedEvent<PacketCreateLevelObject>& event);
	bool OnPacketTeamScoredReceived(const PacketReceivedEvent<PacketTeamScored>& event);
	bool OnPacketDeleteLevelObjectReceived(const PacketReceivedEvent<PacketDeleteLevelObject>& event);
	bool OnPacketMatchStartReceived(const PacketReceivedEvent<PacketMatchStart>& event);
	bool OnPacketMatchBeginReceived(const PacketReceivedEvent<PacketMatchBegin>& event);
	bool OnPacketGameOverReceived(const PacketReceivedEvent<PacketGameOver>& event);

	bool OnPlayerAliveUpdated(const PlayerAliveUpdatedEvent& event);

private:
	bool m_ClientSideBegun = false;
	bool m_HasStarted = false;

	float32 m_CountdownHideTimer = 0.0f;
	

	GUID_Lambda m_CountdownSoundEffects[5];
	GUID_Lambda m_CountdownDoneSoundEffect;
};