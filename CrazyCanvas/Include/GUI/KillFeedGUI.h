#pragma once

#include "Containers/String.h"
#include "Containers/TArray.h"

#include "LambdaEngine.h"

#include <NoesisGUI/Include/NsGui/Storyboard.h>
#include <NoesisGUI/Include/NsGui/StackPanel.h>
#include <NoesisGUI/Include/NsGui/Textblock.h>
#include <NoesisGUI/Include/NsGui/DoubleAnimation.h>
#include <NoesisGUI/Include/NsGui/UserControl.h>

#include "Time/API/Timestamp.h"

#include <unordered_map>

typedef std::pair<Noesis::Ptr<Noesis::TextBlock>, float32> TextFeedTimer;

class KillFeedGUI : public Noesis::UserControl
{
public:
	KillFeedGUI();
	~KillFeedGUI();

	void InitGUI();

	bool ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler) override;

	void UpdateFeedTimer(LambdaEngine::Timestamp delta);

	void AddToKillFeed(const LambdaEngine::String& feedMessage, uint8 killedPlayerTeamIndex);

private:
	void RemoveFromKillFeed(Noesis::TextBlock* textblock);

private:
	//Noesis::Storyboard* m_pKillFeedStoryBoard			= nullptr;
	//Noesis::DoubleAnimation* m_pKillFeedDoubleAnimation	= nullptr;
	Noesis::StackPanel* m_pKillFeedStackPanel			= nullptr;

	uint16 m_FeedIndex = 0;

	LambdaEngine::TArray<TextFeedTimer> m_KillFeedTimers;

	NS_IMPLEMENT_INLINE_REFLECTION_(KillFeedGUI, Noesis::UserControl, "CrazyCanvas.KillFeedGUI")

};