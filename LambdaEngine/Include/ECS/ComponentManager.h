#pragma once

#include "ComponentArray.h"
#include "Component.h"
#include "Defines.h"

namespace LambdaEngine
{
	class ComponentManager
	{
	public:
		DECL_UNIQUE_CLASS(ComponentManager);
		ComponentManager();
		~ComponentManager();

		template<typename Comp>
		void RegisterComponentType();

		template<typename Comp>
		Comp& AddComponent(Entity entity, Comp component);

		template<typename Comp>
		void RemoveComponent(Entity entity);

		template<typename Comp>
		Comp& GetComponent(Entity entity);

		void EntityDeleted(Entity entity);

		template<typename Comp>
		bool HasType();

	private:
		std::unordered_map<std::type_index, uint32> m_CompTypeToArrayMap;

		TArray<IComponentArray*> m_ComponentArrays;
	};

	template<typename Comp>
	inline void ComponentManager::RegisterComponentType()
	{
		std::type_index id = TID(Comp);
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) == m_CompTypeToArrayMap.end(), "Trying to register a component that already exists!");

		m_CompTypeToArrayMap[id] = m_ComponentArrays.GetSize();
		ComponentArray<Comp>* compArray = DBG_NEW ComponentArray<Comp>();
		m_ComponentArrays.PushBack(compArray);
	}

	template<typename Comp>
	inline Comp& ComponentManager::AddComponent(Entity entity, Comp component)
	{
		std::type_index id = TID(Comp);
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) == m_CompTypeToArrayMap.end(), "Trying to add a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		uint32 index = m_CompTypeToArrayMap[id];
		ComponentArray<Comp>* compArray = dynamic_cast<ComponentArray<Comp>*>(m_ComponentArrays[index]);
		
		// Add the new component.
		return compArray->Insert(entity, component);
	}

	template<typename Comp>
	inline void ComponentManager::RemoveComponent(Entity entity)
	{
		std::type_index id = TID(Comp);
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) == m_CompTypeToArrayMap.end(), "Trying to remove a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		uint32 index = m_CompTypeToArrayMap[id];
		ComponentArray<Comp>* compArray = dynamic_cast<ComponentArray<Comp>*>(m_ComponentArrays[index]);

		// Add the new component.
		compArray->Remove(entity);
	}

	template<typename Comp>
	inline Comp& ComponentManager::GetComponent(Entity entity)
	{
		std::type_index id = TID(Comp);
		VALIDATE_MSG(m_CompTypeToArrayMap.find(id) == m_CompTypeToArrayMap.end(), "Trying to fetch a component which was not registered!");

		// Fetch the corresponding ComponentArray for that component type.
		uint32 index = m_CompTypeToArrayMap[id];
		ComponentArray<Comp>* compArray = dynamic_cast<ComponentArray<Comp>*>(m_ComponentArrays[index]);

		return compArray->GetData(entity);
	}

	template<typename Comp>
	inline bool ComponentManager::HasType()
	{
		std::type_index id = TID(Comp);
		return m_CompTypeToArrayMap.find(id) != m_CompTypeToArrayMap.end();
	}
}