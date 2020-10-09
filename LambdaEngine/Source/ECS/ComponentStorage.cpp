#include "ECS/ComponentStorage.h"

namespace LambdaEngine
{
	ComponentStorage::~ComponentStorage()
	{
		for (IComponentArray* compArr : m_ComponentArrays)
			SAFEDELETE(compArr);
		m_ComponentArrays.Clear();
		m_ComponentArraysWithDirtyFlags.Clear();
	}

	void ComponentStorage::UnsetComponentOwner(const ComponentType* pComponentType)
	{
		IComponentArray* pCompArray = GetComponentArray(pComponentType);
		VALIDATE_MSG(pCompArray, "Trying to unset component ownership of an unregistered component type!");
		pCompArray->UnsetComponentOwner();
	}

	bool ComponentStorage::DeleteComponent(Entity entity, const ComponentType* pComponentType)
	{
		IComponentArray* pComponentArray = GetComponentArray(pComponentType);
		if (pComponentArray)
		{
			pComponentArray->Remove(entity);
			return true;
		}
		else
		{
			return false;
		}
	}

	uint32 ComponentStorage::SerializeComponent(Entity entity, const ComponentType* pComponentType, uint8* pBuffer, uint32 bufferSize) const
	{
		const IComponentArray* pComponentArray = GetComponentArray(pComponentType);
		return pComponentArray->SerializeComponent(entity, pBuffer, bufferSize);
	}

	bool ComponentStorage::DeserializeComponent(Entity entity, const ComponentType* pComponentType, uint32 componentDataSize, const uint8* pBuffer, bool& entityHadComponent)
	{
		IComponentArray* pComponentArray = GetComponentArray(pComponentType);
		return pComponentArray->DeserializeComponent(entity, pBuffer, componentDataSize, entityHadComponent);
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

	void ComponentStorage::ResetDirtyFlags()
	{
		for (IComponentArray* pArray : m_ComponentArraysWithDirtyFlags)
		{
			pArray->ResetDirtyFlags();
		}
	}

	const ComponentType* ComponentStorage::GetComponentType(uint32 componentTypeHash) const
	{
		auto componentTypeItr = m_TypeHashToCompTypeMap.find(componentTypeHash);
		return componentTypeItr != m_TypeHashToCompTypeMap.end() ? componentTypeItr->second : nullptr;
	}
}
