#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    ECSCore* ECSCore::s_pInstance = DBG_NEW ECSCore();

    ECSCore::ECSCore()
        :m_EntityPublisher(&m_ComponentManager, &m_EntityRegistry),
        m_DeltaTime(0.0f)
    {}

    void ECSCore::Release()
    {
        DELETE_OBJECT(ECSCore::s_pInstance);
    }

    void ECSCore::Tick(float dt)
    {
        m_DeltaTime = dt;
        PerformEntityDeletions();
        m_JobScheduler.Tick();
    }

    void ECSCore::RemoveEntity(Entity entity)
    {
        m_ComponentManager.EntityDeleted(entity);
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
            {
                // Deregister entity's component from systems
                m_EntityPublisher.UnpublishComponent(entities[entityIdx], componentType);

                // Delete the component
                m_ComponentManager.GetComponentArray(componentType)->Remove(entity);
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
            const std::unordered_set<std::type_index>& typeSet = entityComponentSets[entityIdx];

            for (std::type_index componentType : typeSet)
            {
                m_EntityPublisher.PublishComponent(entities[entityIdx], componentType);
            }
        }
    }

    void ECSCore::PerformEntityDeletions()
    {
        const EntityRegistryPage& registryPage = m_EntityRegistry.GetTopRegistryPage();

        for (Entity entity : m_EntitiesToDelete)
        {
            // Delete every component belonging to the entity
            const auto& componentTypes = registryPage.IndexID(entity);
            for (std::type_index componentType : componentTypes)
            {
                // Delete the component
                IComponentArray* pComponentArray = m_ComponentManager.GetComponentArray(componentType);
                pComponentArray->Remove(entity);

                // Notify systems that the component has been removed
                m_EntityPublisher.UnpublishComponent(entity, componentType);
            }

            // Free the entity ID
            m_EntityRegistry.DeregisterEntity(entity);
        }

        m_EntitiesToDelete.Clear();
    }
}
