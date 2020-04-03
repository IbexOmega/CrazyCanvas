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

		FORCEINLINE float AsSeconds() const
		{
			constexpr float SECONDS = 1000.0f * 1000.0f * 1000.0f;
			return float(m_NanoSeconds) / SECONDS;
		}

		FORCEINLINE float AsMilliSeconds() const
		{
			constexpr float MILLISECONDS = 1000.0f * 1000.0f;
			return float(m_NanoSeconds) / MILLISECONDS;
		}

		FORCEINLINE float AsMicroSeconds() const
		{
			constexpr float MICROSECONDS = 1000.0f;
			return float(m_NanoSeconds) / MICROSECONDS;
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

		FORCEINLINE static Timestamp Seconds(float s)
		{
			constexpr float SECOND = 1000.0f * 1000.0f * 1000.0f;
			return Timestamp(uint64(s * SECOND));
		}

		FORCEINLINE static Timestamp MilliSeconds(float ms)
		{
			constexpr float MILLISECOND = 1000.0f * 1000.0f;
			return Timestamp(uint64(ms * MILLISECOND));
		}

		FORCEINLINE static Timestamp MicroSeconds(float us)
		{
			constexpr float MICROSECOND = 1000.0f;
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