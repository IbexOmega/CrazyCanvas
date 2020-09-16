#include "ECS/ComponentStorage.h"

namespace LambdaEngine
{
	ComponentStorage::~ComponentStorage()
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			SAFEDELETE(compArr);
		m_ComponentArrays.Clear();
	}

	void ComponentStorage::EntityDeleted(Entity entity)
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			compArr->EntityDestroyed(entity);
	}

	IComponentArray* ComponentStorage::GetComponentArray(std::type_index componentType)
	{
		auto arrayItr = m_CompTypeToArrayMap.find(componentType);
		return arrayItr == m_CompTypeToArrayMap.end() ? nullptr : m_ComponentArrays[arrayItr->second];
	}

	const IComponentArray* ComponentStorage::GetComponentArray(std::type_index componentType) const
	{
		auto arrayItr = m_CompTypeToArrayMap.find(componentType);
		return arrayItr == m_CompTypeToArrayMap.end() ? nullptr : m_ComponentArrays[arrayItr->second];
	}
}
