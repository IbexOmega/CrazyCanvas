#pragma once

#include "Containers/String.h"
#include "Utilities/StringHash.h"

namespace LambdaEngine
{
	class ComponentType
	{
	public:
		constexpr ComponentType::ComponentType(const ConstString& className) :
			m_ClassName(className)
		{
			
		}

		constexpr uint32 ComponentType::GetHash() const
		{
			return m_ClassName.Hash;
		}

		constexpr const char* ComponentType::GetName() const
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

}

namespace std
{
	template<>
	struct hash<const LambdaEngine::ComponentType*>
	{
		size_t operator()(const LambdaEngine::ComponentType* componentType) const
		{
			return componentType->GetHash();
		}
	};
}
