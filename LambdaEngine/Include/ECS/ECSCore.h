#pragma once

#include "ECS/ComponentStorage.h"
#include "ECS/EntityPublisher.h"
#include "ECS/EntityRegistry.h"
#include "ECS/JobScheduler.h"
#include "ECS/System.h"
#include "Utilities/IDGenerator.h"

#include <typeindex>

namespace LambdaEngine
{
    class EntitySubscriber;
    class RegularWorker;

    class LAMBDA_API ECSCore
    {
    public:
        ECSCore();
        ~ECSCore() = default;

        ECSCore(const ECSCore& other) = delete;
        void operator=(const ECSCore& other) = delete;

        static void Release();

        void Tick(Timestamp deltaTime);

        Entity CreateEntity() { return m_EntityRegistry.CreateEntity(); }

        // Add a component to a specific entity.
        template<typename Comp>
        Comp& AddComponent(Entity entity, const Comp& component);

        // Fetch a reference to a component within a specific entity.
        template<typename Comp>
        Comp& GetComponent(Entity entity);

        // Fetch the whole array with the specific component type.
        template<typename Comp>
        ComponentArray<Comp>* GetComponentArray();

        // Remove a component from a specific entity.
        template<typename Comp>
        bool RemoveComponent(Entity entity);

        // Remove a specific entity.
        void RemoveEntity(Entity entity);

        void ScheduleJobASAP(const Job& job);
        void ScheduleJobPostFrame(const Job& job);

        void AddRegistryPage();
        void DeregisterTopRegistryPage();
        void DeleteTopRegistryPage();
        void ReinstateTopRegistryPage();

        Timestamp GetDeltaTime() const { return m_DeltaTime; }

    public:
        static ECSCore* GetInstance() { return s_pInstance; }

    protected:
        friend EntitySubscriber;
		uint32 SubscribeToEntities(const EntitySubscriberRegistration& subscriberRegistration) { return m_EntityPublisher.SubscribeToEntities(subscriberRegistration); }
		void UnsubscribeFromEntities(uint32 subscriptionID) { m_EntityPublisher.UnsubscribeFromEntities(subscriptionID); }

        friend RegularWorker;
        uint32 ScheduleRegularJob(const Job& job, uint32_t phase)   { return m_JobScheduler.ScheduleRegularJob(job, phase); };
        void DescheduleRegularJob(uint32_t phase, uint32 jobID)     { m_JobScheduler.DescheduleRegularJob(phase, jobID); };

	private:
        void PerformEntityDeletions();

    private:
        EntityRegistry m_EntityRegistry;
        EntityPublisher m_EntityPublisher;
        JobScheduler m_JobScheduler;
        ComponentStorage m_ComponentStorage;

        TArray<Entity> m_EntitiesToDelete;

        // DeltaTime is the time between frames. The typical 'dt' that is passed to update()
        Timestamp m_DeltaTime;

    private:
        static ECSCore* s_pInstance;
    };

    template<typename Comp>
    inline Comp& ECSCore::AddComponent(Entity entity, const Comp& component)
    {
        if (!m_ComponentStorage.HasType<Comp>())
            m_ComponentStorage.RegisterComponentType<Comp>();

        Comp& comp = m_ComponentStorage.AddComponent<Comp>(entity, component);

        m_EntityRegistry.RegisterComponentType(entity, Comp::s_TID);
        m_EntityPublisher.PublishComponent(entity, Comp::s_TID);

        return comp;
    }

    template<typename Comp>
    inline Comp& ECSCore::GetComponent(Entity entity)
    {
        return m_ComponentStorage.GetComponent<Comp>(entity);
    }

    template<typename Comp>
    inline ComponentArray<Comp>* ECSCore::GetComponentArray()
    {
        return m_ComponentStorage.GetComponentArray<Comp>();
    }

    template<typename Comp>
    inline bool ECSCore::RemoveComponent(Entity entity)
    {
        if (m_ComponentStorage.HasType<Comp>())
        {
            m_ComponentStorage.RemoveComponent<Comp>(entity);
            m_EntityRegistry.DeregisterComponentType(entity, componentType);
        	m_EntityPublisher.UnpublishComponent(entity, componentType);
            return true;
        }
        return false;
    }
}
