#pragma once
#include "LambdaEngine.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace LambdaEngine
{
	class SpinLockRecursive
	{
	public:
		SpinLockRecursive() : 
			m_Counter(0)
		{

		}

		FORCEINLINE void lock() noexcept
		{
			if (m_ThreadId == std::this_thread::get_id())
			{
				m_Counter++;
				return;
			}

			while (m_Flag.test_and_set(std::memory_order_acquire));

			m_ThreadId = std::this_thread::get_id();
		}

		FORCEINLINE void unlock() noexcept
		{
			if (m_ThreadId == std::this_thread::get_id())
			{
				if (--m_Counter == 0)
				{
					m_Flag.clear(std::memory_order_release);
				}
			}		
		}

		FORCEINLINE bool try_lock() noexcept
		{
			if (m_ThreadId == std::this_thread::get_id())
			{
				m_Counter++;
				return true;
			}

			if (!m_Flag.test_and_set(std::memory_order_acquire))
			{
				m_ThreadId = std::this_thread::get_id();
				return true;
			}

			return false;
		}

	private:
		std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
		std::thread::id m_ThreadId;
		int m_Counter;
	};
}