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

		bool DeleteComponent(Entity entity, const ComponentType* pComponentType);

		template<typename Comp>
		bool HasType() const;

		bool HasType(const ComponentType* pComponentType) const;

		IComponentArray* GetComponentArray(const ComponentType* pComponentType);
		const IComponentArray* GetComponentArray(const ComponentType* pComponentType) const;

		template<typename Comp>
		ComponentArray<Comp>* GetComponentArray();

	private:
		std::unordered_map<const ComponentType*, uint32> m_CompTypeToArrayMap;

		TArray<IComponentArray*> m_ComponentArrays;
	};

	template<typename Comp>
	inline ComponentArray<Comp>* ComponentStorage::RegisterComponentType()
	{
		const ComponentType* pComponentType = Comp::Type();
		VALIDATE_MSG(m_CompTypeToArrayMap.find(pComponentType) == m_CompTypeToArrayMap.end(), "Trying to register a component that already exists!");

		m_CompTypeToArrayMap[pComponentType] = m_ComponentArrays.GetSize();
		ComponentArray<Comp>* pCompArray = DBG_NEW ComponentArray<Comp>();
		m_ComponentArrays.PushBack(pCompArray);
		return pCompArray;
	}

	template <typename Comp>
	inline void ComponentStorage::SetComponentOwner(const ComponentOwnership<Comp>& componentOwnership)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		if (!pCompArray)
			pCompArray = RegisterComponentType<Comp>();

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

	template<typename Comp>
	inline Comp& ComponentStorage::GetComponent(Entity entity)
	{
		ComponentArray<Comp>* pCompArray = GetComponentArray<Comp>();
		VALIDATE_MSG(pCompArray, "Trying to fetch a component which was not registered!");

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
}
