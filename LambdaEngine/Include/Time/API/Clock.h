#pragma once
#include "Timestamp.h"
#include "PlatformTime.h"

namespace LambdaEngine
{
	class Clock
	{
	public:
		Clock();
		Clock(Clock&& other)		noexcept;
		Clock(const Clock& other)	noexcept;
		~Clock() = default;

		Clock& operator=(Clock&& other)			noexcept;
		Clock& operator=(const Clock& other)	noexcept;

		void Tick();

		FORCEINLINE void Reset()
		{
			m_DeltaTime = Timestamp(0);
			m_TotalTime = Timestamp(0);
		}

		FORCEINLINE const Timestamp& GetDeltaTime() const
		{
			return m_DeltaTime;
		}

		FORCEINLINE const Timestamp& GetTotalTime() const
		{
			return m_TotalTime;
		}

	public:
		FORCEINLINE static Timestamp CurrentTime()
		{
			return Timestamp(PlatformTime::Nanoseconds());
		}

	private:
		Timestamp m_LastTime	= Timestamp(0);
		Timestamp m_TotalTime	= Timestamp(0);
		Timestamp m_DeltaTime	= Timestamp(0);
	};
}