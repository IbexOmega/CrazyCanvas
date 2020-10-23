#include "Time/API/Clock.h"
#include "Time/API/PlatformTime.h"

namespace LambdaEngine
{
	Clock::Clock()
		: m_LastTime(0)
		, m_TotalTime(0)
		, m_DeltaTime(0)
	{
		Tick();
	}

	void Clock::Tick()
	{
		const uint64 now	= PlatformTime::GetPerformanceCounter();
		const uint64 delta	= now - m_LastTime;

		constexpr uint64 NANOSECONDS = 1000 * 1000 * 1000;
		const uint64 frequency		= PlatformTime::GetPerformanceFrequency();
		const uint64 nanoseconds	= (delta * NANOSECONDS) / frequency;

		ASSERT(m_LastTime < now);

		m_DeltaTime = Timestamp(nanoseconds);
		m_LastTime	= now;
		m_TotalTime += m_DeltaTime;
	}
	
	void Clock::Reset()
	{
		// Set lasttime in reset so that next deltatime does not become huge
		m_LastTime = PlatformTime::GetPerformanceCounter();

		m_TotalTime = Timestamp(0);
		m_DeltaTime = Timestamp(0);
	}
}
