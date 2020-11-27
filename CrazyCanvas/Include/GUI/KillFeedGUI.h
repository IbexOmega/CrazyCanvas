#pragma once

#include "Containers/String.h"
#include "Containers/TArray.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/StackPanel.h>
#include <NoesisGUI/Include/NsGui/Grid.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

#include "Time/API/Timestamp.h"

#include <unordered_map>

typedef std::pair<Noesis::Ptr<Noesis::Grid>, float32> TextFeedTimer;

class KillFeedGUI : public Noesis::UserControl
{
public:
	KillFeedGUI();
	~KillFeedGUI();

	void InitGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void UpdateFeedTimer(LambdaEngine::Timestamp delta);

	void AddToKillFeed(const LambdaEngine::String& killed, const LambdaEngine::String& killer, const uint8 killedPlayerTeamIndex);

private:
	void RemoveFromKillFeed(Noesis::Grid* textblock);

private:
	Noesis::StackPanel* m_pKillFeedStackPanel			= nullptr;

	uint16 m_FeedIndex = 0;

	LambdaEngine::TArray<TextFeedTimer> m_KillFeedTimers;

	NS_IMPLEMENT_INLINE_REFLECTION_(KillFeedGUI, Noesis::UserControl, "CrazyCanvas.KillFeedGUI")

};