#pragma once
#include "LambdaEngine.h"

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4251)
#endif

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
			other.m_NanoSeconds = 0;
		}

		FORCEINLINE Timestamp(const Timestamp& other)
			: m_NanoSeconds(other.m_NanoSeconds)
		{
		}

		FORCEINLINE float64 AsSeconds() const
		{
			constexpr float64 SECONDS = 1000.0 * 1000.0 * 1000.0;
			return float64(m_NanoSeconds) / SECONDS;
		}

		FORCEINLINE float64 AsMilliSeconds() const
		{
			constexpr float64 MILLISECONDS = 1000.0 * 1000.0;
			return float64(m_NanoSeconds) / MILLISECONDS;
		}

		FORCEINLINE float64 AsMicroSeconds() const
		{
			constexpr float64 MICROSECONDS = 1000.0;
			return float64(m_NanoSeconds) / MICROSECONDS;
		}

		FORCEINLINE uint64 AsNanoSeconds() const
		{
			return m_NanoSeconds;
		}

		FORCEINLINE Timestamp& operator=(const Timestamp& other)
		{
			m_NanoSeconds = other.m_NanoSeconds;
			return *this;
		}

		FORCEINLINE Timestamp& operator=(Timestamp&& other) noexcept
		{
			m_NanoSeconds = other.m_NanoSeconds;
			other.m_NanoSeconds = 0;
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

		FORCEINLINE static Timestamp Seconds(float64 s)
		{
			constexpr float64 SECOND = 1000.0 * 1000.0 * 1000.0;
			return Timestamp(uint64(s * SECOND));
		}

		FORCEINLINE static Timestamp MilliSeconds(float64 ms)
		{
			constexpr float64 MILLISECOND = 1000.0 * 1000.0;
			return Timestamp(uint64(ms * MILLISECOND));
		}

		FORCEINLINE static Timestamp MicroSeconds(float64 us)
		{
			constexpr float64 MICROSECOND = 1000.0;
			return Timestamp(uint64(us * MICROSECOND));
		}

		FORCEINLINE static Timestamp NanoSeconds(uint64 ns)
		{
			return Timestamp(ns);
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

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif