#pragma once

#include "Application/API/Events/NetworkEvents.h"

#include "Match/MatchBase.h"

class Match
{
public:
	DECL_STATIC_CLASS(Match);

	static bool Init();
	static bool Release();

	static bool CreateMatch(const MatchDescription* pMatch);
	static bool ResetMatch();
	static bool ReleaseMatch();

	static bool OnPacketReceived(const LambdaEngine::PacketReceivedEvent& event);

	FORCEINLINE MatchBase* GetInstance() { return s_pMatchInstance; };

private:
	inline static MatchBase* s_pMatchInstance = nullptr;
};
