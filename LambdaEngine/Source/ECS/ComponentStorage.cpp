#include "ECS/ComponentStorage.h"

namespace LambdaEngine
{
	ComponentStorage::~ComponentStorage()
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			SAFEDELETE(compArr);
		m_ComponentArrays.Clear();
	}

	bool ComponentStorage::DeleteComponent(Entity entity, const ComponentType* pComponentType)
	{
		IComponentArray* pComponentArray = GetComponentArray(pComponentType);
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

	IComponentArray* ComponentStorage::GetComponentArray(const ComponentType* pComponentType)
	{
		auto arrayItr = m_CompTypeToArrayMap.find(pComponentType);
		return arrayItr == m_CompTypeToArrayMap.end() ? nullptr : m_ComponentArrays[arrayItr->second];
	}

	const IComponentArray* ComponentStorage::GetComponentArray(const ComponentType* pComponentType) const
	{
		auto arrayItr = m_CompTypeToArrayMap.find(pComponentType);
		return arrayItr == m_CompTypeToArrayMap.end() ? nullptr : m_ComponentArrays[arrayItr->second];
	}
}
