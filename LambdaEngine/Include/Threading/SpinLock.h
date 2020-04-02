#pragma once
#include <atomic>

class SpinLock
{
public:
	inline void lock() noexcept
	{
		while (m_Flag.test_and_set(std::memory_order_acquire));
	}

	inline void unlock() noexcept
	{
		m_Flag.clear(std::memory_order_release);
	}

	inline bool try_lock() noexcept
	{
		return !m_Flag.test_and_set(std::memory_order_acquire);
	}

	inline bool IsLocked() noexcept
	{
		return m_Flag._My_flag;
	}

private:
	std::atomic_flag m_Flag = ATOMIC_FLAG_INIT;
};