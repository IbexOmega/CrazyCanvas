#include "ECS/ECSCore.h"

#include "Debug/Profiler.h"

namespace LambdaEngine
{
	ECSCore* ECSCore::s_pInstance = DBG_NEW ECSCore();

	ECSCore::ECSCore() :
		m_EntityPublisher(&m_ComponentStorage, &m_EntityRegistry)
	,	m_ECSVisualizer(&m_JobScheduler)
	{}

	void ECSCore::Release()
	{
		SAFEDELETE(ECSCore::s_pInstance);
	}

	void ECSCore::Tick(Timestamp deltaTime)
	{
		m_DeltaTime = deltaTime;
		PROFILE_FUNCTION("ECSCore::PerformComponentRegistrations", PerformComponentRegistrations());
		PROFILE_FUNCTION("ECSCore::PerformComponentDeletions", PerformComponentDeletions());
		PROFILE_FUNCTION("ECSCore::PerformEntityDeletions", PerformEntityDeletions());
		PROFILE_FUNCTION("m_JobScheduler.Tick", m_JobScheduler.Tick((float32)deltaTime.AsSeconds()));
		m_ComponentStorage.ResetDirtyFlags();

#ifdef LAMBDA_DEVELOPMENT
		m_ECSVisualizer.Render();
#endif
	}

	IComponentArray* ECSCore::GetComponentArray(const ComponentType* pComponentType)
	{
		return m_ComponentStorage.GetComponentArray(pComponentType);
	}

	const IComponentArray* ECSCore::GetComponentArray(const ComponentType* pComponentType) const
	{
		return m_ComponentStorage.GetComponentArray(pComponentType);
	}

	void ECSCore::RemoveEntity(Entity entity)
	{
		std::scoped_lock<SpinLock> lock(m_LockRemoveEntity);
		m_EntitiesToDelete.insert(entity);
	}

	void ECSCore::ScheduleJobASAP(const Job& job)
	{
		m_JobScheduler.ScheduleJobASAP(job);
	}

	void ECSCore::ScheduleJobPostFrame(const Job& job)
	{
		m_JobScheduler.ScheduleJob(job, LAST_PHASE + 1u);
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
		TArray<const ComponentType*> componentTypes;

		uint32 entityCount = entities.GetSize();
		for (uint32 entityNr = 0; entityNr < entityCount; entityNr++)
		{
			Entity entity = entities[entityNr];
			const std::unordered_set<const ComponentType*>& typeSet = entityComponentSets[entityNr];
			componentTypes.Assign(typeSet.begin(), typeSet.end());

			for (const ComponentType* pComponentType : componentTypes)
				m_EntityRegistry.DeregisterComponentType(entity, pComponentType);

			for (const ComponentType* pComponentType : componentTypes)
			{
				m_EntityPublisher.UnpublishComponent(entity, pComponentType);
				m_ComponentStorage.DeleteComponent(entity, pComponentType);
			}
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

	uint32 ECSCore::SerializeEntity(Entity entity, const TArray<const ComponentType*>& componentsFilter, uint8* pBuffer, uint32 bufferSize) const
	{
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
		uint32 serializedComponentsCount = 0u;
		for (const ComponentType* pComponentType : componentsFilter)
		{
			const IComponentArray* pComponentArray = m_ComponentStorage.GetComponentArray(pComponentType);
			if (pComponentArray && !pComponentArray->HasComponent(entity))
			{
				LOG_WARNING("Attempted to serialize a component type which entity %d does not have: %s", entity, pComponentType->GetName());
				continue;
			}

			const uint32 requiredComponentSize = m_ComponentStorage.SerializeComponent(entity, pComponentType, pBuffer, remainingSize);
			requiredTotalSize += requiredComponentSize;
			if (requiredComponentSize <= remainingSize)
			{
				pBuffer += requiredComponentSize;
				remainingSize -= requiredComponentSize;
				++serializedComponentsCount;
			}
		}

		// Finalize the serialization by writing the header
		if (hasRoomForHeader)
		{
			const EntitySerializationHeader header =
			{
				.TotalSerializationSize	= requiredTotalSize,
				.Entity					= entity,
				.ComponentCount			= serializedComponentsCount
			};

			memcpy(pHeaderPosition, &header, headerSize);
		}

		return requiredTotalSize;
	}

	bool ECSCore::DeserializeEntity(const uint8* pBuffer)
	{
		constexpr const uint32 entityHeaderSize = sizeof(EntitySerializationHeader);
		EntitySerializationHeader entityHeader;
		memcpy(&entityHeader, pBuffer, entityHeaderSize);
		pBuffer += entityHeaderSize;

		ASSERT_MSG(m_EntityRegistry.GetTopRegistryPage().HasElement(entityHeader.Entity), "Attempted to deserialize unknown entity: %d", entityHeader.Entity);

		// Deserialize each component. If the entity already has the component, update its data. Otherwise, create it.
		const uint32 componentCount = entityHeader.ComponentCount;
		bool success = true;
		for (uint32 componentIdx = 0u; componentIdx < componentCount; ++componentIdx)
		{
			constexpr const uint32 componentHeaderSize = sizeof(ComponentSerializationHeader);
			ComponentSerializationHeader componentHeader;
			memcpy(&componentHeader, pBuffer, componentHeaderSize);
			pBuffer += componentHeaderSize;

			const ComponentType* pComponentType = m_ComponentStorage.GetComponentType(componentHeader.TypeHash);
			if (!pComponentType)
			{
				LOG_WARNING("Attempted to deserialize an unregistered component type, hash: %d", componentHeader.TypeHash);
				success = false;
				continue;
			}

			bool entityHadComponent = false;
			const uint32 componentDataSize = componentHeader.TotalSerializationSize - componentHeaderSize;
			success = m_ComponentStorage.DeserializeComponent(entityHeader.Entity, pComponentType, componentDataSize, pBuffer, entityHadComponent) && success;

			if (!entityHadComponent)
			{
				m_ComponentsToRegister.PushBack({entityHeader.Entity, pComponentType});
			}
		}

		return success;
	}

	void ECSCore::RegisterSystem(System* pSystem, uint32 regularJobID)
	{
		m_Systems.PushBack(pSystem, regularJobID);
	}

	void ECSCore::DeregisterSystem(uint32 regularJobID)
	{
		m_Systems.Pop(regularJobID);
	}

	void ECSCore::PerformComponentRegistrations()
	{
		// Register all components first, then publish them
		for (const std::pair<Entity, const ComponentType*>& component : m_ComponentsToRegister)
		{
			m_EntityRegistry.RegisterComponentType(component.first, component.second);
		}

		for (const std::pair<Entity, const ComponentType*>& component : m_ComponentsToRegister)
		{
			m_EntityPublisher.PublishComponent(component.first, component.second);
		}

		m_ComponentsToRegister.ShrinkToFit();
		m_ComponentsToRegister.Clear();
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
		/*	The component types to delete of each entity. It is a copy of the entity's set of component types in
			the entity registry. Copying the set is necessary as the set is popped each time it is iterated. */
		TArray<const ComponentType*> componentTypes;

		for (Entity entity : m_EntitiesToDelete)
		{
			if (registryPage.HasElement(entity))
			{
				// Delete every component belonging to the entity
				const std::unordered_set<const ComponentType*>& componentTypesSet = registryPage.IndexID(entity);
				componentTypes.Assign(componentTypesSet.begin(), componentTypesSet.end());

				for (const ComponentType* pComponentType : componentTypes)
					m_EntityRegistry.DeregisterComponentType(entity, pComponentType);

				for (const ComponentType* pComponentType : componentTypes)
				{
					m_EntityPublisher.UnpublishComponent(entity, pComponentType);
					m_ComponentStorage.DeleteComponent(entity, pComponentType);
				}

				// Free the entity ID
				m_EntityRegistry.DeregisterEntity(entity);
			}
		}

		m_EntitiesToDelete.clear();
	}

	bool ECSCore::DeleteComponent(Entity entity, const ComponentType* pComponentType)
	{
		m_EntityRegistry.DeregisterComponentType(entity, pComponentType);
		m_EntityPublisher.UnpublishComponent(entity, pComponentType);
		return m_ComponentStorage.DeleteComponent(entity, pComponentType);
	}
}
