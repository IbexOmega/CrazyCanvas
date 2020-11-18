#pragma once

#include "Containers/String.h"
#include "Utilities/StringHash.h"

namespace LambdaEngine
{
	class ComponentType
	{
	public:
		constexpr ComponentType(const ConstString& className) :
			m_ClassName(className)
		{

		}

		constexpr uint32 GetHash() const
		{
			return m_ClassName.Hash;
		}

		constexpr const char* GetName() const
		{
			return m_ClassName.Name;
		}

		bool operator==(const ComponentType& other) const
		{
			return strcmp(m_ClassName.Name, other.m_ClassName.Name) == 0;
		}

	private:
		const ConstString m_ClassName;
	};

	/*	ComponentTypeHash allows one to use an already hashed uint32 as a key in an unordered associative container.
		This is used to for a <uint32, ComponentType> mapping, which is used when deserializing entities. When sending entity data over the internet,
		only the hash of each component type is sent, not its name string. Hence the mapping is needed. */
	class ComponentTypeHash
	{
	public:
		ComponentTypeHash(uint32 hash) :
			Hash(hash)
		{}

		bool operator==(const ComponentTypeHash& other) const
		{
			return Hash == other.Hash;
		}

		const uint32 Hash;
	};
}

namespace std
{
	template<>
	struct hash<const LambdaEngine::ComponentType*>
	{
		size_t operator()(const LambdaEngine::ComponentType* pComponentType) const
		{
			return pComponentType->GetHash();
		}
	};

	template<>
	struct hash<LambdaEngine::ComponentTypeHash>
	{
		size_t operator()(const LambdaEngine::ComponentTypeHash& typeHash) const
		{
			return typeHash.Hash;
		}
	};
}
