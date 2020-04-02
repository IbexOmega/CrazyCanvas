#pragma once
#include "LambdaEngine.h"

namespace LambdaEngine
{
	class Timestamp
	{
	public:
		FORCEINLINE Timestamp(uint64 nanoseconds = 0)
			: m_NanoSeconds(nanoseconds)
		{
		}

		FORCEINLINE Timestamp(Timestamp&& other)
			: m_NanoSeconds(other.m_NanoSeconds)
		{
		}

		FORCEINLINE Timestamp(const Timestamp& other)
			: m_NanoSeconds(other.m_NanoSeconds)
		{
		}

		FORCEINLINE uint64 AsSeconds() const
		{
			constexpr uint64 SECONDS = 1000 * 1000 * 1000;
			return uint64(m_NanoSeconds) / SECONDS;
		}

		FORCEINLINE uint64 AsMilliSeconds() const
		{
			constexpr uint64 MILLISECONDS = 1000 * 1000;
			return uint64(m_NanoSeconds) / MILLISECONDS;
		}

		FORCEINLINE uint64 AsMicroSeconds() const
		{
			constexpr uint64 MICROSECONDS = 1000;
			return uint64(m_NanoSeconds) / MICROSECONDS;
		}

		FORCEINLINE uint64 AsNanoSeconds() const
		{
			return uint64(m_NanoSeconds);
		}

		FORCEINLINE Timestamp& operator=(const Timestamp& other)
		{
			m_NanoSeconds = other.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE bool operator==(const Timestamp& other) const
		{
			return m_NanoSeconds == other.m_NanoSeconds;
		}

		FORCEINLINE bool operator!=(const Timestamp& other) const
		{
			return m_NanoSeconds != other.m_NanoSeconds;
		}

		FORCEINLINE Timestamp& operator+=(const Timestamp& right)
		{
			m_NanoSeconds += right.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE Timestamp& operator-=(const Timestamp& right)
		{
			m_NanoSeconds -= right.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE Timestamp& operator*=(const Timestamp& right)
		{
			m_NanoSeconds *= right.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE Timestamp& operator/=(const Timestamp& right)
		{
			m_NanoSeconds /= right.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE static Timestamp Seconds(uint64 s)
		{
			constexpr uint64 SECOND = 1000 * 1000 * 1000;
			return Timestamp(uint64(s * SECOND));
		}

		FORCEINLINE static Timestamp MilliSeconds(uint64 ms)
		{
			constexpr uint64 MILLISECOND = 1000 * 1000;
			return Timestamp(uint64(ms * MILLISECOND));
		}

		FORCEINLINE static Timestamp MicroSeconds(uint64 us)
		{
			constexpr uint64 MICROSECOND = 1000;
			return Timestamp(uint64(us * MICROSECOND));
		}

		FORCEINLINE static Timestamp NanoSeconds(uint64 ns)
		{
			return Timestamp(uint64(ns));
		}

		friend Timestamp operator+(const Timestamp& left, const Timestamp& right);
		friend Timestamp operator-(const Timestamp& left, const Timestamp& right);
		friend Timestamp operator*(const Timestamp& left, const Timestamp& right);
		friend Timestamp operator/(const Timestamp& left, const Timestamp& right);
		friend bool operator>(const Timestamp& left, const Timestamp& right);
		friend bool operator<(const Timestamp& left, const Timestamp& right);
		friend bool operator>=(const Timestamp& left, const Timestamp& right);
		friend bool operator<=(const Timestamp& left, const Timestamp& right);

	private:
		uint64 m_NanoSeconds;
	};

	FORCEINLINE Timestamp operator+(const Timestamp& left, const Timestamp& right)
	{
		return Timestamp(left.m_NanoSeconds + right.m_NanoSeconds);
	}

	FORCEINLINE Timestamp operator-(const Timestamp& left, const Timestamp& right)
	{
		return Timestamp(left.m_NanoSeconds - right.m_NanoSeconds);
	}

	FORCEINLINE Timestamp operator*(const Timestamp& left, const Timestamp& right)
	{
		return Timestamp(left.m_NanoSeconds * right.m_NanoSeconds);
	}

	FORCEINLINE Timestamp operator/(const Timestamp& left, const Timestamp& right)
	{
		return Timestamp(left.m_NanoSeconds / right.m_NanoSeconds);
	}

	FORCEINLINE bool operator>(const Timestamp& left, const Timestamp& right)
	{
		return left.m_NanoSeconds > right.m_NanoSeconds;
	}

	FORCEINLINE bool operator<(const Timestamp& left, const Timestamp& right)
	{
		return left.m_NanoSeconds < right.m_NanoSeconds;
	}

	FORCEINLINE bool operator>=(const Timestamp& left, const Timestamp& right)
	{
		return (left.m_NanoSeconds >= right.m_NanoSeconds);
	}

	FORCEINLINE bool operator<=(const Timestamp& left, const Timestamp& right)
	{
		return (left.m_NanoSeconds <= right.m_NanoSeconds);
	}
}