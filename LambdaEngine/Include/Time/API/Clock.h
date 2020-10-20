#pragma once
#include "Timestamp.h"

namespace LambdaEngine
{
	class LAMBDA_API Clock
	{
	public:
		Clock();
		~Clock() = default;

		/*
		* Measures the deltatime between this and the latest call to Clock::Tick. It also updates the totalTime that the clock
		* has been active. This is the time between the last call to Clock::Reset and this call to Clock::Tick
		*/
		void Tick();
		void Reset();

		FORCEINLINE const Timestamp& GetDeltaTime() const
		{
			return m_DeltaTime;
		}

		FORCEINLINE const Timestamp& GetTotalTime() const
		{
			return m_TotalTime;
		}

	private:
		uint64 m_LastTime;
		Timestamp m_TotalTime;
		Timestamp m_DeltaTime;
	};
}