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
		VALIDATE_MSG(arrayItr != m_CompTypeToArrayMap.end(),
			"Attempted to fetch non-existing component array for component type: %s", componentType.name());

		return m_ComponentArrays[arrayItr->second];
	}

	const IComponentArray* ComponentStorage::GetComponentArray(std::type_index componentType) const
	{
		auto arrayItr = m_CompTypeToArrayMap.find(componentType);
		VALIDATE_MSG(arrayItr != m_CompTypeToArrayMap.end(),
			"Attempted to fetch non-existing component array for component type: %s", componentType.name());

		return m_ComponentArrays[arrayItr->second];
	}
}
