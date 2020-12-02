#pragma once

#include "Types.h"

#include "Containers/String.h"

#include "ECS/Entity.h"

/*
* ClientHost -> GAME_STATE_SETUP -> Server
* ServerBroadcast -> GAME_STATE_SETUP
* Each client -> GAME_STATE_LOADING -> Server
* ServerBroadcast -> CreateGameObjects and players
* ServerBroadcast -> Thats' all
* Each client -> GAME_STATE_LOADED -> Server
* ServerBroadcast -> GAME_STATE_COUNTDOWN
*/

enum EGameState : uint8
{
	GAME_STATE_LOBBY,
	GAME_STATE_SETUP,
	GAME_STATE_LOADING,
	GAME_STATE_LOADED,
	GAME_STATE_COUNTDOWN,
	GAME_STATE_PLAYING,
	GAME_STATE_GAME_OVER
};

class Player
{
	friend class PlayerManagerBase;
	friend class PlayerManagerClient;
	friend class PlayerManagerServer;

public:
	Player(const Player& other);

	const LambdaEngine::String& GetName() const;
	LambdaEngine::Entity GetEntity() const;
	bool IsHost() const;
	bool IsDead() const;
	bool IsReady() const;
	uint16 GetPing() const;
	EGameState GetState() const;
	uint8 GetTeam() const;
	uint8 GetKills() const;
	uint8 GetDeaths() const;
	uint8 GetFlagsCaptured() const;
	uint8 GetFlagsDefended() const;
	uint64 GetUID() const;

	bool operator==(const Player& other) const;
	bool operator!=(const Player& other) const;
	bool operator<(const Player& other) const;

	Player& operator=(const Player& other);

private:
	Player();

public:
	static inline constexpr const char* GameStateToString(EGameState gameState)
	{
		switch (gameState)
		{
		case EGameState::GAME_STATE_LOBBY:		return "Lobby";
		case EGameState::GAME_STATE_SETUP:		return "Setup";
		case EGameState::GAME_STATE_LOADING:	return "Loading";
		case EGameState::GAME_STATE_LOADED:		return "Loaded";
		case EGameState::GAME_STATE_COUNTDOWN:	return "Countdown";
		case EGameState::GAME_STATE_PLAYING:	return "Playing";
		case EGameState::GAME_STATE_GAME_OVER:	return "Gameover";
		default: return "NONE";
		}
	}

private:
	LambdaEngine::String m_Name;
	LambdaEngine::Entity m_Entity;
	bool m_IsHost;
	bool m_IsDead;
	bool m_IsReady;
	uint16 m_Ping;
	EGameState m_State;
	uint8 m_Team;
	uint8 m_Kills;
	uint8 m_Deaths;
	uint8 m_FlagsCaptured;
	uint8 m_FlagsDefended;
	uint64 m_UID;
};