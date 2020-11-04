#include "Lobby/Player.h"

using namespace LambdaEngine;

Player::Player() : 
    m_Name(),
    m_Ping(),
    m_Kills(0),
    m_Deaths(0),
    m_FlagsCaptured(0),
    m_FlagsDefended(0),
    m_UID(0)
{

}

const String& Player::GetName() const
{
    return m_Name;
}

Timestamp Player::GetPing() const
{
    return m_Ping;
}

uint16 Player::GetKills() const
{
    return m_Kills;
}

uint16 Player::GetDeaths() const
{
    return m_Deaths;
}

uint16 Player::GetFlagsCaptured() const
{
    return m_FlagsCaptured;
}

uint16 Player::GetFlagsDefended() const
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
