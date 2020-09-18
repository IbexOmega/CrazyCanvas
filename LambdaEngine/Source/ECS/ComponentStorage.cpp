#include "ECS/ComponentStorage.h"

namespace LambdaEngine
{
	ComponentStorage::~ComponentStorage()
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			SAFEDELETE(compArr);
		m_ComponentArrays.Clear();
	}

	bool ComponentStorage::DeleteComponent(Entity entity, std::type_index componentType)
	{
		IComponentArray* pComponentArray = GetComponentArray(componentType);
		if (pComponentArray)
		{
			pComponentArray->DeleteEntity(entity);
			return true;
		}
		else
		{
			return false;
		}
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
