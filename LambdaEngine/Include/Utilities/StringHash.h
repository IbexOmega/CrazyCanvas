#pragma once
#include "HashUtilities.h"

namespace LambdaEngine
{
	constexpr uint64 PRIME_MULTIPLE = 16777619ULL;
	constexpr uint32 INITIAL_HASH	= 2166136261U;

	/*
	* Compile-time hash
	*/
	template <size_t INDEX>
	class Hash
	{
	public:
		template <size_t STRLEN>
		constexpr static uint32 Generate(const char(&str)[STRLEN])
		{
			using THash = Hash<INDEX - 1u>;
			return static_cast<uint32>(static_cast<uint64>(THash::Generate(str) ^ (uint32)(str[INDEX - 1u]))* PRIME_MULTIPLE);
		}
	};

	template <>
	class Hash<0u>
	{
	public:
		template <size_t STRLEN>
		constexpr static uint32 Generate(const char(&str)[STRLEN])
		{
			UNREFERENCED_VARIABLE(str);
			return INITIAL_HASH;
		}
	};

	template <typename  T>
	class HashHelper {};

	template <>
	class HashHelper<const char*>
	{
	private:
		static uint32 Fnv1aHash(const char* str, unsigned int hash = INITIAL_HASH)
		{
			while (*str != 0)
			{
				hash = (*str ^ hash) * PRIME_MULTIPLE;
				str++;
			}

			return hash;
		}

	public:
		static uint32 Generate(const char* str)
		{
			return Fnv1aHash(str);
		}
	};

	template <size_t STRLEN>
	class HashHelper<char[STRLEN]>
	{
	public:
		static constexpr uint32 Generate(const char(&str)[STRLEN])
		{
			return Hash<STRLEN - 1u>::Generate(str);
		}
	};

	/*
	* ConstString
	*/
	template <class Type>
	constexpr uint32 HashString(const Type& str)
	{
		return HashHelper<Type>::Generate(str);
	}

	struct ConstString
	{
		template<uint32 N>
		constexpr ConstString(const char(&name)[N]) :
			Name(name),
			Hash(HashString(name))
		{
		}

		const char* Name;
		uint32		Hash;
	};
}
