#include "Time/API/Clock.h"

namespace LambdaEngine
{
	Clock::Clock()
	{
		Tick();
	}

	Clock::Clock(Clock&& other) noexcept
		: m_LastTime(other.m_LastTime),
		m_TotalTime(other.m_TotalTime),
		m_DeltaTime(other.m_DeltaTime)
	{
		other.m_DeltaTime	= 0;
		other.m_TotalTime	= 0;
		other.m_LastTime	= 0;
	}

	Clock::Clock(const Clock& other) noexcept
		: m_LastTime(other.m_LastTime),
		m_TotalTime(other.m_TotalTime),
		m_DeltaTime(other.m_DeltaTime)
	{
	}

	Clock& Clock::operator=(Clock&& other) noexcept
	{
		if (this != &other)
		{
			m_DeltaTime = other.m_DeltaTime;
			m_TotalTime = other.m_TotalTime;
			m_LastTime	= other.m_LastTime;
			other.m_DeltaTime	= 0;
			other.m_TotalTime	= 0;
			other.m_LastTime	= 0;
		}

		return *this;
	}

	Clock& Clock::operator=(const Clock& other) noexcept
	{
		if (this != &other)
		{
			m_DeltaTime = other.m_DeltaTime;
			m_TotalTime = other.m_TotalTime;
			m_LastTime	= other.m_LastTime;
		}

		return *this;
	}

	void Clock::Tick()
	{
		uint64		nanoseconds	= PlatformTime::Nanoseconds();
		Timestamp	now			= Timestamp(nanoseconds);

		m_DeltaTime = now - m_LastTime;
		m_LastTime	= now;
		m_TotalTime += m_DeltaTime;
	}
}