#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    ECSCore ECSCore::s_Instance;

    ECSCore::ECSCore()
        :m_EntityPublisher(&m_EntityRegistry),
        m_ECSBooter(this),
        m_DeltaTime(0.0f)
    {}

    void ECSCore::Tick(float dt)
    {
        m_DeltaTime = dt;

        PerformEntityDeletions();
        PerformRegistrations();

        m_JobScheduler.Tick();
    }

    void ECSCore::RemoveEntity(Entity entity)
    {
        m_ComponentStorage.EntityDeleted(entity);
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

    void ECSCore::EnqueueComponentHandlerRegistration(const ComponentHandlerRegistration& handlerRegistration)
    {
        m_ECSBooter.EnqueueComponentHandlerRegistration(handlerRegistration);
    }

    void ECSCore::EnqueueComponentSubscriberRegistration(const Subscriber& subscriber)
    {
        m_ECSBooter.EnqueueSubscriberInitialization(subscriber);
    }

    void ECSCore::PerformRegistrations()
    {
        // Initialize and register systems and component handlers
        m_ECSBooter.PerformBootups();
    }

    void ECSCore::PerformEntityDeletions()
    {
        THashTable<std::type_index, ComponentKalle>& componentStorage = m_EntityPublisher.GetComponentStorage();
        const EntityRegistryPage& registryPage = m_EntityRegistry.GetTopRegistryPage();

        for (Entity entity : m_EntitiesToDelete)
        {
            // Delete every component belonging to the entity
            const auto& componentTypes = registryPage.IndexID(entity);
            for (std::type_index componentType : componentTypes)
            {
                // Delete the component
                auto containerItr = componentStorage.find(componentType);
                ComponentKalle& component = containerItr->second;

                if (component.ComponentDestructor != nullptr)
                {
                    component.ComponentDestructor(entity);
                }

                component.pContainer->Pop(entity);

                // Notify systems that the component has been removed
                m_EntityPublisher.RemovedComponent(entity, componentType);
            }

            // Free the entity ID
            m_EntityRegistry.DeregisterEntity(entity);
        }

        m_EntitiesToDelete.Clear();
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

        for (uint32 i = 0; i < entities.GetSize(); i++)
        {
            const std::unordered_set<std::type_index>& typeSet = entityComponentSets[i];

            for (std::type_index type : typeSet) {
                // Deregister entity's components from systems
                m_EntityPublisher.RemovedComponent(entities[i], type);
            }
        }
    }

    void ECSCore::DeleteTopRegistryPage()
    {
        const EntityRegistryPage& page = m_EntityRegistry.GetTopRegistryPage();
        const auto& entityComponentSets = page.GetVec();
        const TArray<Entity>& entities = page.GetIDs();

        THashTable<std::type_index, ComponentKalle>& componentStorage = m_EntityPublisher.GetComponentStorage();

        for (uint32 i = 0; i < entities.GetSize(); i++)
        {
            const std::unordered_set<std::type_index>& typeSet = entityComponentSets[i];

            for (std::type_index componentType : typeSet)
            {
                // Deregister entity's component from systems
                m_EntityPublisher.RemovedComponent(entities[i], componentType);

                // Delete the component
                auto containerItr = componentStorage.find(componentType);
                ComponentKalle& component = containerItr->second;

                if (component.ComponentDestructor != nullptr)
                {
                    component.ComponentDestructor(entities[i]);
                }

                component.pContainer->Pop(entities[i]);
            }
        }

        m_EntityRegistry.RemovePage();
    }

    void ECSCore::ReinstateTopRegistryPage()
    {
        const EntityRegistryPage& page = m_EntityRegistry.GetTopRegistryPage();

        const auto& entityComponentSets = page.GetVec();
        const TArray<Entity>& entities = page.GetIDs();

        for (uint32 i = 0; i < entities.GetSize(); i++)
        {
            const std::unordered_set<std::type_index>& typeSet = entityComponentSets[i];

            for (std::type_index componentType : typeSet)
            {
                m_EntityPublisher.NewComponent(entities[i], componentType);
            }
        }
    }

    void ECSCore::ComponentAdded(Entity entity, std::type_index componentType)
    {
        m_EntityRegistry.RegisterComponentType(entity, componentType);
        m_EntityPublisher.NewComponent(entity, componentType);
    }

    void ECSCore::ComponentDeleted(Entity entity, std::type_index componentType)
    {
        m_EntityRegistry.DeregisterComponentType(entity, componentType);
        m_EntityPublisher.RemovedComponent(entity, componentType);
    }

    void ECSCore::EnqueueEntitySubscriptions(const EntitySubscriberRegistration& subscriberRegistration, const std::function<bool()>& initFn, uint32* pSubscriberID)
    {
        Subscriber subscriber = {
            .ComponentSubscriptions = subscriberRegistration,
            .InitFunction           = initFn,
            .pSubscriberID          = pSubscriberID
        };

        m_ECSBooter.EnqueueSubscriberInitialization(subscriber);
    }
}
