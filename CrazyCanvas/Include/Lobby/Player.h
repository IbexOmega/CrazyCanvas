#pragma once

#include "Types.h"

#include "Containers/String.h"

#include "Time/API/Timestamp.h"

class Player
{
	friend class PlayerManager;

public:
	const LambdaEngine::String& GetName() const;
	LambdaEngine::Timestamp GetPing() const;
	uint16 GetKills() const;
	uint16 GetDeaths() const;
	uint16 GetFlagsCaptured() const;
	uint16 GetFlagsDefended() const;
	uint64 GetUID() const;

	bool operator==(const Player& other) const;
	bool operator!=(const Player& other) const;
	bool operator<(const Player& other) const;

private:
	Player();

private:
	LambdaEngine::String m_Name;
	LambdaEngine::Timestamp m_Ping;
	uint16 m_Kills;
	uint16 m_Deaths;
	uint16 m_FlagsCaptured;
	uint16 m_FlagsDefended;
	uint64 m_UID;
};