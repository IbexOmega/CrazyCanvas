#pragma once
#include "Timestamp.h"

namespace LambdaEngine
{
	class LAMBDA_API Clock
	{
	public:
		Clock();
		Clock(Clock&& other)		noexcept;
		Clock(const Clock& other)	noexcept;
		~Clock() = default;

		Clock& operator=(Clock&& other)			noexcept;
		Clock& operator=(const Clock& other)	noexcept;

		/*
		* Measures the deltatime between this and the latest call to Clock::Tick. It also updates the totalTime that the clock
		* has been active. This is the time between the last call to Clock::Reset and this call to Clock::Tick
		*/
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

	private:
		uint64 m_LastTime = 0;

		Timestamp m_TotalTime	= Timestamp(0);
		Timestamp m_DeltaTime	= Timestamp(0);
	};
}
