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

		inline PrehashedString(const char* pString)
			: m_Str(String(pString))
			, m_Hash(CalculateHash())
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

		inline String& GetString()
		{
			return m_Str;
		}

		inline const String& GetString() const
		{
			return m_Str;
		}

		inline const char* GetCString() const
		{
			return m_Str.c_str();
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

		inline bool operator==(const char* pStr) const
		{
			return (strcmp(m_Str.c_str(), pStr) == 0);
		}

		inline bool operator!=(const char* pStr) const
		{
			return !(*this == pStr);
		}

		inline PrehashedString& operator=(const char* pNewString)
		{
			m_Str	= String(pNewString);
			m_Hash	= CalculateHash();
			return *this;
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