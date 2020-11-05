#pragma once

#include "Types.h"

#include "Containers/String.h"

#include "ECS/Entity.h"

enum EPlayerState : uint8
{
	PLAYER_STATE_LOBBY,
	PLAYER_STATE_READY,
	PLAYER_STATE_LOADING,
	PLAYER_STATE_PLAYING,
	PLAYER_STATE_DEAD,
};

class Player
{
	friend class PlayerManagerBase;
	friend class PlayerManagerClient;
	friend class PlayerManagerServer;

public:
	const LambdaEngine::String& GetName() const;
	LambdaEngine::Entity GetEntity() const;
	uint16 GetPing() const;
	EPlayerState GetState() const;
	uint8 GetTeam() const;
	uint8 GetKills() const;
	uint8 GetDeaths() const;
	uint8 GetFlagsCaptured() const;
	uint8 GetFlagsDefended() const;
	uint64 GetUID() const;

	bool operator==(const Player& other) const;
	bool operator!=(const Player& other) const;
	bool operator<(const Player& other) const;

private:
	Player();

private:
	LambdaEngine::String m_Name;
	LambdaEngine::Entity m_Entity;
	uint16 m_Ping;
	EPlayerState m_State;
	uint8 m_Team;
	uint8 m_Kills;
	uint8 m_Deaths;
	uint8 m_FlagsCaptured;
	uint8 m_FlagsDefended;
	uint64 m_UID;
};