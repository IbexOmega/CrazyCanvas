#pragma once

#include "ECS/ComponentHandler.h"
#include "ECS/EntityPublisher.h"
#include "ECS/ECSBooter.h"
#include "ECS/EntityRegistry.h"
#include "ECS/JobScheduler.h"
#include "ECS/System.h"

#include "Utilities/IDGenerator.h"

#include <typeindex>

#define TID(type) std::type_index(typeid(type))

namespace LambdaEngine
{
    class EntitySubscriber;
    class RegularWorker;

    class ECSCore
    {
    public:
        ECSCore();
        ~ECSCore() = default;

        ECSCore(const ECSCore& other) = delete;
        void operator=(const ECSCore& other) = delete;

        void Tick(float dt);

        Entity createEntity() { return m_EntityRegistry.CreateEntity(); }

        void ScheduleJobASAP(const Job& job);
        void ScheduleJobPostFrame(const Job& job);

        void EnqueueComponentHandlerRegistration(const ComponentHandlerRegistration& handlerRegistration);
        void EnqueueComponentSubscriberRegistration(const Subscriber& subscriber);

        // Registers and initializes component handlers and entity subscribers
        void PerformRegistrations();

        // Enqueues an entity deletion
        void EnqueueEntityDeletion(Entity entity);
        void PerformEntityDeletions();

        void AddRegistryPage();
        void DeregisterTopRegistryPage();
        void DeleteTopRegistryPage();
        void ReinstateTopRegistryPage();

        void ComponentAdded(Entity entity, std::type_index componentType);
        void ComponentDeleted(Entity entity, std::type_index componentType);

        EntityPublisher* GetEntityPublisher()   { return &m_EntityPublisher; }
        float GetDeltaTime() const              { return m_DeltaTime; }

    public:
        static ECSCore* GetInstance() { return &s_Instance; }

    protected:
        friend EntitySubscriber;
        void EnqueueEntitySubscriptions(const EntitySubscriberRegistration& subscriberRegistration, const std::function<bool()>& initFn, uint32* pSubscriberID);

        friend RegularWorker;
        uint32 ScheduleRegularJob(const Job& job, uint32_t phase)   { return m_JobScheduler.ScheduleRegularJob(job, phase); };
        void DescheduleRegularJob(uint32_t phase, uint32 jobID)     { m_JobScheduler.DescheduleRegularJob(phase, jobID); };

    private:
        EntityRegistry m_EntityRegistry;
        EntityPublisher m_EntityPublisher;
        JobScheduler m_JobScheduler;
        ECSBooter m_ECSBooter;

        TArray<Entity> m_EntitiesToDelete;

        // DeltaTime is the time between frames. The typical 'dt' that is passed to update()
        float m_DeltaTime;

    private:
        static ECSCore s_Instance;
    };
}
