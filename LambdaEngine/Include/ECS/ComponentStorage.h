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
		void RegisterComponentType();

		template<typename Comp>
		Comp& AddComponent(Entity entity, const Comp& component);

		template<typename Comp>
		void RemoveComponent(Entity entity);

		template<typename Comp>
		Comp& GetComponent(Entity entity);

		bool DeleteComponent(Entity entity, std::type_index componentType);

		template<typename Comp>
		bool HasType() const;

		bool HasType(std::type_index componentType) const;

		IComponentArray* GetComponentArray(std::type_index componentType);
		const IComponentArray* GetComponentArray(std::type_index componentType) const;

		template<typename Comp>
		ComponentArray<Comp>* GetComponentArray();

	private:
		std::unordered_map<std::type_index, uint32> m_CompTypeToArrayMap;

		TArray<IComponentArray*> m_ComponentArrays;
	};

	template<typename Comp>
	inline void ComponentStorage::RegisterComponentType()
	{
		std::type_index id = Comp::s_TID;
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) == m_CompTypeToArrayMap.end(), "Trying to register a component that already exists!");

		m_CompTypeToArrayMap[id] = m_ComponentArrays.GetSize();
		ComponentArray<Comp>* compArray = DBG_NEW ComponentArray<Comp>();
		m_ComponentArrays.PushBack(compArray);
	}

	template<typename Comp>
	inline Comp& ComponentStorage::AddComponent(Entity entity, const Comp& component)
	{
		std::type_index id = Comp::s_TID;
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) != m_CompTypeToArrayMap.end(), "Trying to add a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		ComponentArray<Comp>* compArray = GetComponentArray<Comp>();

		// Add the new component.
		return compArray->Insert(entity, component);
	}

	template<typename Comp>
	inline void ComponentStorage::RemoveComponent(Entity entity)
	{
		std::type_index id = Comp::s_TID;
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) != m_CompTypeToArrayMap.end(), "Trying to remove a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		ComponentArray<Comp>* compArray = GetComponentArray<Comp>();

		// Add the new component.
		compArray->Remove(entity);
	}

	template<typename Comp>
	inline Comp& ComponentStorage::GetComponent(Entity entity)
	{
		std::type_index id = Comp::s_TID;
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) != m_CompTypeToArrayMap.end(), "Trying to fetch a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		ComponentArray<Comp>* compArray = GetComponentArray<Comp>();

		return compArray->GetData(entity);
	}

	template<typename Comp>
	inline bool ComponentStorage::HasType() const
	{
		return m_CompTypeToArrayMap.find(Comp::s_TID) != m_CompTypeToArrayMap.end();
	}

	inline bool ComponentStorage::HasType(std::type_index componentType) const
	{
		return m_CompTypeToArrayMap.find(componentType) != m_CompTypeToArrayMap.end();
	}

	template<typename Comp>
	inline ComponentArray<Comp>* ComponentStorage::GetComponentArray()
	{
		return static_cast<ComponentArray<Comp>*>(GetComponentArray(Comp::s_TID));
	}
}
