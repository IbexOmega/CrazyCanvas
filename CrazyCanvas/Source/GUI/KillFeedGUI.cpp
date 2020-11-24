#include "GUI/KillFeedGUI.h"

#include "NoesisPCH.h"

#include "Teams/TeamHelper.h"


KillFeedGUI::KillFeedGUI()
{
	Noesis::GUI::LoadComponent(this, "KillFeedGUI.xaml");

	m_pKillFeedStoryBoard = FindResource<Noesis::Storyboard>("KillFeedStoryBoard");
	m_pKillFeedStackPanel = FindName<Noesis::StackPanel>("KillFeedStackPanel");
}

KillFeedGUI::~KillFeedGUI()
{
}

bool KillFeedGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void KillFeedGUI::UpdateFeedTimer(LambdaEngine::Timestamp delta)
{
	LambdaEngine::TArray<TextFeedTimer> defferedKillFeedTimersToRemove;

	for (TextFeedTimer pair : m_KillFeedTimers)
	{
		pair.second -= float32(delta.AsSeconds());

		if (pair.second <= 0.0f)
			defferedKillFeedTimersToRemove.EmplaceBack(pair);
	}

	for (TextFeedTimer pair : defferedKillFeedTimersToRemove)
	{
		RemoveFromKillFeed(pair.first);
	}
}

void KillFeedGUI::AddToKillFeed(const LambdaEngine::String& feedMessage, uint8 killedPlayerTeamIndex)
{
	LambdaEngine::String name = "feed" + std::to_string(m_FeedIndex);
	
	Noesis::Ptr<Noesis::TextBlock> feed = *new Noesis::TextBlock();
	Noesis::Ptr<Noesis::SolidColorBrush> pBrush = *new Noesis::SolidColorBrush();
	


	uint8 colorIndex = killedPlayerTeamIndex == 0 ? 0 : 1;
	glm::vec3 teamColor = TeamHelper::GetTeamColor(colorIndex);
	Noesis::Color killFeedColor(teamColor.r, teamColor.g, teamColor.b);

	pBrush->SetColor(killFeedColor);

	feed->SetText(feedMessage.c_str());
	feed->SetFontSize(23.0f);
	feed->SetName(name.c_str());
	feed->SetForeground(pBrush);

	TextFeedTimer pair = std::make_pair(feed, 8.0f);

	m_KillFeedTimers.EmplaceBack(pair);

	m_pKillFeedStackPanel->GetChildren()->Add(feed);
	m_pKillFeedStoryBoard->SetTargetName(m_pKillFeedStoryBoard, name.c_str());

	m_pKillFeedStoryBoard->Begin();
}

void KillFeedGUI::RemoveFromKillFeed(Noesis::TextBlock* textblock)
{
	m_pKillFeedStackPanel->GetChildren()->Remove(textblock);
}
