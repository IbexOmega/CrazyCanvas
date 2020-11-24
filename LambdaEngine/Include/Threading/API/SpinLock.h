#pragma once
#include "LambdaEngine.h"

#include <atomic>
#include <mutex>
#include <thread>

#include "Log/Log.h"

namespace LambdaEngine
{
#ifndef LAMBDA_PRODUCTION
	class SpinLock
	{
	public:
		FORCEINLINE void lock() noexcept
		{
			//A crash here means you have a Deadlock :) Tip look at the Call Stack
			ASSERT(m_ThreadId != std::this_thread::get_id());
			while (m_Flag.test_and_set(std::memory_order_acquire));
			m_ThreadId = std::this_thread::get_id();
		}

		FORCEINLINE void unlock() noexcept
		{
			ASSERT(m_ThreadId == std::this_thread::get_id());
			m_Flag.clear(std::memory_order_release);
			m_ThreadId = std::thread::id();
		}

		FORCEINLINE bool try_lock() noexcept
		{
			ASSERT(m_ThreadId != std::this_thread::get_id());
			bool success = !m_Flag.test_and_set(std::memory_order_acquire);
			if (success)
			{
				m_ThreadId = std::this_thread::get_id();
			}
			return success;
		}

	private:
		std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
		std::thread::id m_ThreadId;
};

#else

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
#endif
}