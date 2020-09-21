#include "ECS/ECSCore.h"

namespace LambdaEngine
{
	ECSCore* ECSCore::s_pInstance = DBG_NEW ECSCore();

	ECSCore::ECSCore()
		:m_EntityPublisher(&m_ComponentStorage, &m_EntityRegistry)
	{}

	void ECSCore::Release()
	{
		DELETE_OBJECT(ECSCore::s_pInstance);
	}

	void ECSCore::Tick(Timestamp deltaTime)
	{
		m_DeltaTime = deltaTime;
		PerformComponentRegistrations();
		PerformComponentDeletions();
		PerformEntityDeletions();
		m_JobScheduler.Tick();
	}

	void ECSCore::RemoveEntity(Entity entity)
	{
		m_EntitiesToDelete.PushBack(entity);
	}

	void ECSCore::ScheduleJobASAP(const Job& job)
	{
		m_JobScheduler.ScheduleJob(job, CURRENT_PHASE);
	}

	void ECSCore::ScheduleJobPostFrame(const Job& job)
	{
		m_JobScheduler.ScheduleJob(job, g_LastPhase + 1u);
	}

	void ECSCore::AddRegistryPage()
	{
		m_EntityRegistry.AddPage();
	}

	void ECSCore::DeregisterTopRegistryPage()
	{
		const EntityRegistryPage& page = m_EntityRegistry.GetTopRegistryPage();

		const auto& entityComponentSets = page.GetVec();
		const TArray<Entity>& entities = page.GetIDs();

		for (uint32 entityIdx = 0; entityIdx < entities.GetSize(); entityIdx++)
		{
			const std::unordered_set<std::type_index>& typeSet = entityComponentSets[entityIdx];

			for (std::type_index type : typeSet) {
				// Deregister entity's components from systems
				m_EntityPublisher.UnpublishComponent(entities[entityIdx], type);
			}
		}
	}

	void ECSCore::DeleteTopRegistryPage()
	{
		const EntityRegistryPage& page = m_EntityRegistry.GetTopRegistryPage();
		const auto& entityComponentSets = page.GetVec();
		const TArray<Entity>& entities = page.GetIDs();

		for (uint32 entityIdx = 0; entityIdx < entities.GetSize(); entityIdx++)
		{
			Entity entity = entities[entityIdx];
			const std::unordered_set<std::type_index>& typeSet = entityComponentSets[entityIdx];

			for (std::type_index componentType : typeSet)
				DeleteComponent(entity, componentType);
		}

		m_EntityRegistry.RemovePage();
	}

	void ECSCore::ReinstateTopRegistryPage()
	{
		const EntityRegistryPage& page = m_EntityRegistry.GetTopRegistryPage();

		const auto& entityComponentSets = page.GetVec();
		const TArray<Entity>& entities = page.GetIDs();

		for (uint32 entityIdx = 0; entityIdx < entities.GetSize(); entityIdx++)
		{
			const std::unordered_set<std::type_index>& typeSet = entityComponentSets[entityIdx];

			for (std::type_index componentType : typeSet)
				m_EntityPublisher.PublishComponent(entities[entityIdx], componentType);
		}
	}

	void ECSCore::PerformComponentRegistrations()
	{
		for (const std::pair<Entity, std::type_index>& component : m_ComponentsToRegister)
		{
			m_EntityRegistry.RegisterComponentType(component.first, component.second);
			m_EntityPublisher.PublishComponent(component.first, component.second);
		}

		m_ComponentsToRegister.ShrinkToFit();
		m_ComponentsToRegister.Clear();
	}

	void ECSCore::PerformComponentDeletions()
	{
		for (const std::pair<Entity, std::type_index>& component : m_ComponentsToDelete)
		{
			if (DeleteComponent(component.first, component.second))
			{
				// If the entity has no more components, delete it
				const std::unordered_set<std::type_index>& componentTypes = m_EntityRegistry.GetTopRegistryPage().IndexID(component.first);
				if (componentTypes.empty())
					m_EntityRegistry.DeregisterEntity(component.first);
			}
		}

		m_ComponentsToDelete.ShrinkToFit();
		m_ComponentsToDelete.Clear();
	}

	void ECSCore::PerformEntityDeletions()
	{
		const EntityRegistryPage& registryPage = m_EntityRegistry.GetTopRegistryPage();

		for (Entity entity : m_EntitiesToDelete)
		{
			// Delete every component belonging to the entity
			const std::unordered_set<std::type_index>& componentTypes = registryPage.IndexID(entity);
			for (std::type_index componentType : componentTypes)
				DeleteComponent(entity, componentType);

			// Free the entity ID
			m_EntityRegistry.DeregisterEntity(entity);
		}

		m_EntitiesToDelete.ShrinkToFit();
		m_EntitiesToDelete.Clear();
	}

	bool ECSCore::DeleteComponent(Entity entity, std::type_index componentType)
	{
		if (m_ComponentStorage.DeleteComponent(entity, componentType))
		{
			m_EntityPublisher.UnpublishComponent(entity, componentType);
			return true;
		}

		return false;
	}
}
