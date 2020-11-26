#include "GUI/KillFeedGUI.h"

#include "NoesisPCH.h"
#include "Log/Log.h"

#include "Teams/TeamHelper.h"


KillFeedGUI::KillFeedGUI() :
	m_KillFeedTimers()
{
	Noesis::GUI::LoadComponent(this, "KillFeedGUI.xaml");
}

KillFeedGUI::~KillFeedGUI()
{
}

void KillFeedGUI::InitGUI()
{
	m_pKillFeedStackPanel = FindName<Noesis::StackPanel>("KILL_FEED_STACK_PANEL");
}

bool KillFeedGUI::ConnectEvent(Noesis::BaseComponent* pSource, const char* pEvent, const char* pHandler)
{
	return false;
}

void KillFeedGUI::UpdateFeedTimer(LambdaEngine::Timestamp delta)
{
	if (!m_KillFeedTimers.IsEmpty())
	{
		for (int i = m_KillFeedTimers.GetSize() - 1; i >= 0; i--)
		{
			TextFeedTimer& pair = m_KillFeedTimers[i];
			pair.second -= float32(delta.AsSeconds());

			if (pair.second <= 0.0f)
			{
				RemoveFromKillFeed(pair.first);
				m_KillFeedTimers.Erase(m_KillFeedTimers.Begin() + i);
			}
		}
	}
}

void KillFeedGUI::AddToKillFeed(const LambdaEngine::String& feedMessage, const uint8 killedPlayerTeamIndex)
{
	Noesis::Ptr<Noesis::TextBlock> feed = *new Noesis::TextBlock();
	Noesis::Ptr<Noesis::SolidColorBrush> pBrush = *new Noesis::SolidColorBrush();
	
	uint8 colorIndex = killedPlayerTeamIndex == 0 ? 1 : 0; //will only work for 2 teams, Implement a smarter way to handle this later
	glm::vec3 teamColor = TeamHelper::GetTeamColor(colorIndex);
	Noesis::Color killFeedColor(teamColor.r, teamColor.g, teamColor.b);

	pBrush->SetColor(killFeedColor);

	feed->SetText(feedMessage.c_str());
	feed->SetFontSize(23.0f);
	feed->SetForeground(pBrush);

	TextFeedTimer pair = std::make_pair(feed, 8.0f);

	m_KillFeedTimers.EmplaceBack(pair);
	m_pKillFeedStackPanel->GetChildren()->Add(feed);
}

void KillFeedGUI::RemoveFromKillFeed(Noesis::TextBlock* textblock)
{
	m_pKillFeedStackPanel->GetChildren()->Remove(textblock);
}
