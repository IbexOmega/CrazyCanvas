#pragma once

#include "LambdaEngine.h"
#include "Utilities/SHA256.h"
#include "Containers/String.h"

class Level;

struct MatchDescription
{
	LambdaEngine::SHA256Hash LevelHash;
};

class MatchBase
{
public:
	MatchBase() = default;
	~MatchBase() = default;

	bool Init();

protected:
	virtual bool InitInternal() = 0;

protected:
	Level* m_pLevel = nullptr;
};