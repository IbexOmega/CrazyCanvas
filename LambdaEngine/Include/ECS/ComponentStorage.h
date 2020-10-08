#pragma once

#include "ComponentArray.h"
#include "Component.h"
#include "Defines.h"

namespace LambdaEngine
{
	class ComponentStorage
	{
	public:
		DECL_UNIQUE_CLASS(ComponentStorage);
		ComponentStorage() = default;
		~ComponentStorage();

		template<typename Comp>
		ComponentArray<Comp>* RegisterComponentType();

		template <typename Comp>
		void SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership);
		void UnsetComponentOwner(const ComponentType* pComponentType);

		template<typename Comp>
		Comp& AddComponent(Entity entity, const Comp& component);

		template<typename Comp>
		void RemoveComponent(Entity entity);

		template<typename Comp>
		Comp& GetComponent(Entity entity);

		template<typename Comp>
		const Comp& GetComponent(Entity entity) const;

		bool DeleteComponent(Entity entity, const ComponentType* pComponentType);

		void SerializeComponent(Entity entity, uint8* pBuffer, uint32 bufferSize);

		template <typename Comp>
		uint32 SerializeComponent(const Comp& component, uint8* pBuffer, uint32 bufferSize) const;
		uint32 SerializeComponent(Entity entity, const ComponentType* pComponentType, uint8* pBuffer, uint32 bufferSize) const;

		template<typename Comp>
		bool HasType() const;

		bool HasType(const ComponentType* pComponentType) const;

		void ResetDirtyFlags();

		IComponentArray* GetComponentArray(const ComponentType* pComponentType);
		const IComponentArray* GetComponentArray(const ComponentType* pComponentType) const;

		template<typename Comp>
		ComponentArray<Comp>* GetComponentArray();

		template<typename Comp>
		const ComponentArray<Comp>* GetComponentArray() const;

	private:
		std::unordered_map<const ComponentType*, uint32> m_CompTypeToArrayMap;

		TArray<IComponentArray*> m_ComponentArrays;
		// All component types with dirty flags. Used for resetting dirty flags at the end of each frame.
		TArray<IComponentArray*> m_ComponentArraysWithDirtyFlags;
	};

	template<typename Comp>
	inline ComponentArray<Comp>* ComponentStorage::RegisterComponentType()
	{
		const ComponentType* pComponentType = Comp::Type();
		VALIDATE_MSG(m_CompTypeToArrayMap.find(pComponentType) == m_CompTypeToArrayMap.end(), "Trying to register a component that already exists!");

		m_CompTypeToArrayMap[pComponentType] = m_ComponentArrays.GetSize();
		ComponentArray<Comp>* pCompArray = DBG_NEW ComponentArray<Comp>();
		m_ComponentArrays.PushBack(pCompArray);

		if constexpr (Comp::HasDirtyFlag())
		{
			m_ComponentArraysWithDirtyFlags.PushBack(pCompArray);
		}

		return pCompArray;
	}

	template <typename Comp>
	inline void ComponentStorage::SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		if (!pCompArray)
		{
			pCompArray = RegisterComponentType<Comp>();
		}

		pCompArray->SetComponentOwner(componentOwnership);
	}

	template<typename Comp>
	inline Comp& ComponentStorage::AddComponent(Entity entity, const Comp& component)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		VALIDATE_MSG(pCompArray != nullptr, "Trying to add a component which was not registered!");

		// Add the new component.
		return pCompArray->Insert(entity, component);
	}

	template<typename Comp>
	inline void ComponentStorage::RemoveComponent(Entity entity)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		VALIDATE_MSG(pCompArray != nullptr, "Trying to remove a component which was not registered!");

		// Add the new component.
		pCompArray->Remove(entity);
	}

	template <typename Comp>
	inline uint32 ComponentStorage::SerializeComponent(const Comp& component, uint8* pBuffer, uint32 bufferSize) const
	{
		const ComponentArray<Comp>* pComponentArray = GetComponentArray<Comp>();
		return pComponentArray->SerializeComponent(component, pBuffer, bufferSize);
	}

	template<typename Comp>
	inline Comp& ComponentStorage::GetComponent(Entity entity)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		VALIDATE_MSG(pCompArray, "Trying to fetch an unregistered component type!");

		return pCompArray->GetData(entity);
	}

	template<typename Comp>
	inline const Comp& ComponentStorage::GetComponent(Entity entity) const
	{
		const ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		VALIDATE_MSG(pCompArray, "Trying to fetch an unregistered component type!");

		return pCompArray->GetData(entity);
	}

	template<typename Comp>
	inline bool ComponentStorage::HasType() const
	{
		return m_CompTypeToArrayMap.find(Comp::Type()) != m_CompTypeToArrayMap.end();
	}

	inline bool ComponentStorage::HasType(const ComponentType* pComponentType) const
	{
		return m_CompTypeToArrayMap.find(pComponentType) != m_CompTypeToArrayMap.end();
	}

	template<typename Comp>
	inline ComponentArray<Comp>* ComponentStorage::GetComponentArray()
	{
		return static_cast<ComponentArray<Comp>*>(GetComponentArray(Comp::Type()));
	}

	template<typename Comp>
	inline const ComponentArray<Comp>* ComponentStorage::GetComponentArray() const
	{
		return static_cast<const ComponentArray<Comp>*>(GetComponentArray(Comp::Type()));
	}
}
