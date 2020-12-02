#include "GUI/KillFeedGUI.h"

#include "NoesisPCH.h"
#include "Log/Log.h"

#include "Teams/TeamHelper.h"
#include "GUI/GUIHelpers.h"


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

void KillFeedGUI::AddToKillFeed(const LambdaEngine::String& killed, const LambdaEngine::String& killer, const uint8 killedPlayerTeamIndex)
{
	Noesis::Ptr<Noesis::Grid> grid = *new Noesis::Grid();

	AddColumnDefinition(grid->GetColumnDefinitions(), 1.0f, Noesis::GridUnitType::GridUnitType_Star);
	AddColumnDefinition(grid->GetColumnDefinitions(), 1.0f, Noesis::GridUnitType::GridUnitType_Star);
	AddColumnDefinition(grid->GetColumnDefinitions(), 1.0f, Noesis::GridUnitType::GridUnitType_Star);

	Noesis::Ptr<Noesis::TextBlock> killerFeed	= *new Noesis::TextBlock();
	Noesis::Ptr<Noesis::TextBlock> feed			= *new Noesis::TextBlock();
	Noesis::Ptr<Noesis::TextBlock> killedFeed	= *new Noesis::TextBlock();

	Noesis::Ptr<Noesis::SolidColorBrush> killerFeedBrush = *new Noesis::SolidColorBrush();
	Noesis::Ptr<Noesis::SolidColorBrush> feedBrush = *new Noesis::SolidColorBrush();
	Noesis::Ptr<Noesis::SolidColorBrush> killedFeedBrush = *new Noesis::SolidColorBrush();
	Noesis::Ptr<Noesis::SolidColorBrush> strokeBrush = *new Noesis::SolidColorBrush();

	uint8 killerPlayerIndex = killedPlayerTeamIndex == 1 ? 2 : 1;
	uint8 killedPlayerIndex = killedPlayerTeamIndex;

	glm::vec3 killerTeamColor = TeamHelper::GetTeamColor(killerPlayerIndex);
	glm::vec3 killedTeamColor = TeamHelper::GetTeamColor(killedPlayerIndex);

	Noesis::Color killerFeedColor(killerTeamColor.r, killerTeamColor.g, killerTeamColor.b);
	Noesis::Color feedColor(Noesis::Color::White());
	Noesis::Color killedFeedColor(killedTeamColor.r, killedTeamColor.g, killedTeamColor.b);
	Noesis::Color StrokeColor(Noesis::Color::Black());

	killerFeedBrush->SetColor(killerFeedColor);
	feedBrush->SetColor(feedColor);
	killedFeedBrush->SetColor(killedFeedColor);
	strokeBrush->SetColor(StrokeColor);

	killerFeed->SetText(killer.c_str());
	feed->SetText(" killed ");
	killedFeed->SetText(killed.c_str());

	killerFeed->SetForeground(killerFeedBrush);
	feed->SetForeground(feedBrush);
	killedFeed->SetForeground(killedFeedBrush);

	Noesis::Style* pStyle = FrameworkElement::FindResource<Noesis::Style>("KillFeedTextStyle");

	killerFeed->SetStyle(pStyle);
	feed->SetStyle(pStyle);
	killedFeed->SetStyle(pStyle);

	grid->GetChildren()->Add(killerFeed);
	grid->GetChildren()->Add(feed);
	grid->GetChildren()->Add(killedFeed);

	grid->SetColumn(killerFeed, 0);
	grid->SetColumn(feed,		1);
	grid->SetColumn(killedFeed, 2);

	TextFeedTimer pair = std::make_pair(grid, 8.0f);

	m_KillFeedTimers.EmplaceBack(pair);
	m_pKillFeedStackPanel->GetChildren()->Add(grid);
}

void KillFeedGUI::RemoveFromKillFeed(Noesis::Grid* pGrid)
{
	m_pKillFeedStackPanel->GetChildren()->Remove(pGrid);
}
