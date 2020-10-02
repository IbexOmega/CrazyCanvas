#pragma once
#include "String.h"

namespace LambdaEngine
{
	/*
	* PrehashedString
	*/

	class PrehashedString
	{
	public:
		inline PrehashedString()
			: m_Str()
			, m_Hash()
		{
		}

		inline PrehashedString(const String& string)
			: m_Str(string)
			, m_Hash(CalculateHash())
		{
		}

		inline PrehashedString(const PrehashedString& other)
			: m_Str(other.m_Str)
			, m_Hash(other.CalculateHash())
		{
		}

		inline PrehashedString(PrehashedString&& other)
			: m_Str(std::move(other.m_Str))
			, m_Hash(0)
		{
			CalculateHash();
			other.m_Hash = 0;
		}

		inline size_t GetHash() const
		{
			if (m_Hash == 0)
			{
				m_Hash = CalculateHash();
			}

			return m_Hash;
		}

		inline bool operator==(const PrehashedString& other) const
		{
			if (GetHash() != other.GetHash())
			{
				return false;
			}

			return m_Str == other.m_Str;
		}

		inline bool operator!=(const PrehashedString& other) const
		{
			return !(*this == other);
		}

		inline bool operator==(const String& str) const
		{
			return m_Str == str;
		}

		inline bool operator!=(const String& str) const
		{
			return !(*this == str);
		}

		inline PrehashedString& operator=(const String& newString)
		{
			m_Str	= newString;
			m_Hash	= CalculateHash();
			return *this;
		}

		inline PrehashedString& operator=(const PrehashedString& other)
		{
			m_Str	= other.m_Str;
			m_Hash	= CalculateHash();
			return *this;
		}

		inline PrehashedString& operator=(PrehashedString&& other)
		{
			m_Str			= std::move(other.m_Str);
			m_Hash			= CalculateHash();
			other.m_Hash	= 0;
			return *this;
		}

		inline operator String() const
		{
			return m_Str;
		}

	private:
		inline size_t CalculateHash() const
		{
			std::hash<String> hasher;
			return hasher(m_Str);
		}

	private:
		String m_Str;
		mutable size_t m_Hash;
	};

	/*
	* PrehashedStringHasher
	*/

	struct PrehashedStringHasher
	{
		inline size_t operator()(const PrehashedString& prehashedString) const
		{
			return prehashedString.GetHash();
		}
	};
}