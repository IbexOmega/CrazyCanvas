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
		uint64 now		= PlatformTime::GetPerformanceCounter();
		uint64 delta	= now - m_LastTime;

		constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
		uint64 frequency	= PlatformTime::GetPerformanceFrequency();
		uint64 nanoseconds	= (delta * NANOSECONDS) / frequency;

		ASSERT(m_LastTime < now);

		m_DeltaTime = Timestamp(nanoseconds);
		m_LastTime	= now;
		m_TotalTime += m_DeltaTime;
	}
}