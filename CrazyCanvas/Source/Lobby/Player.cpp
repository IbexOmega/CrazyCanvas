#include "Lobby/Player.h"

using namespace LambdaEngine;

Player::Player()
	: m_Name()
	, m_Entity(UINT32_MAX)
	, m_IsHost(false)
	, m_IsDead(false)
	, m_IsReady(false)
	, m_Ping()
	, m_State(GAME_STATE_LOBBY)
	, m_Team(0)
	, m_Kills(0)
	, m_Deaths(0)
	, m_FlagsCaptured(0)
	, m_FlagsDefended(0)
	, m_UID(0)
{
}

Player::Player(const Player& other)
	: m_Name(other.m_Name)
	, m_Entity(other.m_Entity)
	, m_IsHost(other.m_IsHost)
	, m_IsDead(other.m_IsDead)
	, m_IsReady(other.m_IsReady)
	, m_Ping(other.m_Ping)
	, m_State(other.m_State)
	, m_Team(other.m_Team)
	, m_Kills(other.m_Kills)
	, m_Deaths(other.m_Deaths)
	, m_FlagsCaptured(other.m_FlagsCaptured)
	, m_FlagsDefended(other.m_FlagsDefended)
	, m_UID(other.m_UID)
{
}

const String& Player::GetName() const
{
	return m_Name;
}

LambdaEngine::Entity Player::GetEntity() const
{
	return m_Entity;
}

bool Player::IsHost() const
{
	return m_IsHost;
}

bool Player::IsDead() const
{
	return m_IsDead;
}

bool Player::IsReady() const
{
	return m_IsReady;
}

uint16 Player::GetPing() const
{
	return m_Ping;
}

EGameState Player::GetState() const
{
	return m_State;
}

uint8 Player::GetTeam() const
{
	return m_Team;
}

uint8 Player::GetKills() const
{
	return m_Kills;
}

uint8 Player::GetDeaths() const
{
	return m_Deaths;
}

uint8 Player::GetFlagsCaptured() const
{
	return m_FlagsCaptured;
}

uint8 Player::GetFlagsDefended() const
{
	return m_FlagsDefended;
}

uint64 Player::GetUID() const
{
	return m_UID;
}

bool Player::operator==(const Player& other) const
{
	return m_UID == other.m_UID;
}

bool Player::operator!=(const Player& other) const
{
	return m_UID != other.m_UID;
}

bool Player::operator<(const Player& other) const
{
	return m_UID < other.m_UID;
}

Player& Player::operator=(const Player& other)
{
	if (this != &other)
	{
		m_Name			= other.m_Name;
		m_Entity		= other.m_Entity;
		m_IsHost		= other.m_IsHost;
		m_IsDead		= other.m_IsDead;
		m_IsReady		= other.m_IsReady;
		m_Ping			= other.m_Ping;
		m_State			= other.m_State;
		m_Team			= other.m_Team;
		m_Kills			= other.m_Kills;
		m_Deaths		= other.m_Deaths;
		m_FlagsCaptured	= other.m_FlagsCaptured;
		m_FlagsDefended	= other.m_FlagsDefended;
		m_UID			= other.m_UID;
	}

	return *this;
}