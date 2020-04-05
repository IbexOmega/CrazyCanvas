#pragma once
#include "LambdaEngine.h"

#include <atomic>
#include <mutex>

namespace LambdaEngine
{
	class SpinLock
	{
	public:
		FORCEINLINE void lock() noexcept
		{
			while (m_Flag.test_and_set(std::memory_order_acquire));
		}

		FORCEINLINE void unlock() noexcept
		{
			m_Flag.clear(std::memory_order_release);
		}

		FORCEINLINE bool try_lock() noexcept
		{
			return !m_Flag.test_and_set(std::memory_order_acquire);
		}

	private:
		std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
	};
}