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
		m_ComponentStorage.ResetDirtyFlags();
	}

	void ECSCore::RemoveEntity(Entity entity)
	{
		std::scoped_lock<SpinLock> lock(m_LockRemoveEntity);
		m_EntitiesToDelete.PushBack(entity);
	}

	void ECSCore::ScheduleJobASAP(const Job& job)
	{
		m_JobScheduler.ScheduleJobASAP(job);
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
			const std::unordered_set<const ComponentType*>& typeSet = entityComponentSets[entityIdx];

			for (const ComponentType* pComponentType : typeSet)
			{
				// Deregister entity's components from systems
				m_EntityPublisher.UnpublishComponent(entities[entityIdx], pComponentType);
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
			const std::unordered_set<const ComponentType*>& typeSet = entityComponentSets[entityIdx];

			for (const ComponentType* pComponentType : typeSet)
				DeleteComponent(entity, pComponentType);
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
			const std::unordered_set<const ComponentType*>& typeSet = entityComponentSets[entityIdx];

			for (const ComponentType* pComponentType : typeSet)
				m_EntityPublisher.PublishComponent(entities[entityIdx], pComponentType);
		}
	}

	uint32 ECSCore::SerializeEntity(Entity entity, uint8* pBuffer, uint32 bufferSize) const
	{
		const EntityRegistryPage& entityPage = m_EntityRegistry.GetTopRegistryPage();
		const std::unordered_set<const ComponentType*>& componentTypes = entityPage.IndexID(entity);

		/*	EntitySerializationHeader is written to the beginning of the buffer. This is done last, when the size of
			the serialization is known. */
		uint8* pHeaderPosition = pBuffer;

		uint32 remainingSize = bufferSize;
		constexpr const uint32 headerSize = sizeof(EntitySerializationHeader);
		const bool hasRoomForHeader = bufferSize >= headerSize;
		if (hasRoomForHeader)
		{
			pBuffer			+= headerSize;
			remainingSize	-= headerSize;
		}

		// Serialize all components
		uint32 requiredTotalSize = headerSize;
		for (const ComponentType* pComponentType : componentTypes)
		{
			const uint32 requiredComponentSize = m_ComponentStorage.SerializeComponent(entity, pComponentType, pBuffer, remainingSize);
			requiredTotalSize += requiredComponentSize;
			if (requiredComponentSize <= remainingSize)
			{
				pBuffer += requiredComponentSize;
				remainingSize -= requiredComponentSize;
			}
		}

		// Finalize the serialization by writing the header
		if (hasRoomForHeader)
		{
			const EntitySerializationHeader header =
			{
				.TotalSerializationSize	= requiredTotalSize,
				.Entity					= entity,
				.ComponentCount			= (uint32)componentTypes.size()
			};

			memcpy(pHeaderPosition, &header, headerSize);
		}

		return requiredTotalSize;
	}

	uint32 ECSCore::SerializeComponent(Entity entity, const ComponentType* pComponentType, uint8* pBuffer, uint32 bufferSize) const
	{
		return m_ComponentStorage.SerializeComponent(entity, pComponentType, pBuffer, bufferSize);
	}

	void ECSCore::PerformComponentRegistrations()
	{
		for (const std::pair<Entity, const ComponentType*>& component : m_ComponentsToPublish)
		{
			m_EntityPublisher.PublishComponent(component.first, component.second);
		}

		m_ComponentsToPublish.ShrinkToFit();
		m_ComponentsToPublish.Clear();
	}

	void ECSCore::PerformComponentDeletions()
	{
		for (const std::pair<Entity, const ComponentType*>& component : m_ComponentsToDelete)
		{
			if (DeleteComponent(component.first, component.second))
			{
				// If the entity has no more components, delete it
				const std::unordered_set<const ComponentType*>& componentTypes = m_EntityRegistry.GetTopRegistryPage().IndexID(component.first);
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
			const std::unordered_set<const ComponentType*>& componentTypes = registryPage.IndexID(entity);
			for (const ComponentType* pComponentType : componentTypes)
				DeleteComponent(entity, pComponentType);

			// Free the entity ID
			m_EntityRegistry.DeregisterEntity(entity);
		}

		m_EntitiesToDelete.ShrinkToFit();
		m_EntitiesToDelete.Clear();
	}

	bool ECSCore::DeleteComponent(Entity entity, const ComponentType* pComponentType)
	{
		m_EntityPublisher.UnpublishComponent(entity, pComponentType);
		return m_ComponentStorage.DeleteComponent(entity, pComponentType);
	}
}
