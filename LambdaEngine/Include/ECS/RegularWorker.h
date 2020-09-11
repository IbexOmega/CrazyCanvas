#pragma once

#include "ECS/EntitySubscriber.h"
#include "ECS/Job.h"

namespace LambdaEngine
{
    class ECSCore;

    // RegularWorker schedules a regular job and deregisters it upon destruction
    class RegularWorker
    {
    public:
        RegularWorker(ECSCore* pECS);
        ~RegularWorker();

        void ScheduleRegularWork(const Job& job, uint32 phase);

    protected:
        // GetUniqueComponentAccesses serializes all unique component accesses in an entity subscriber registration
        static TArray<ComponentAccess> GetUniqueComponentAccesses(const EntitySubscriberRegistration& subscriberRegistration);

    private:
        static void MapComponentAccesses(const TArray<ComponentAccess>& componentAccesses, THashTable<std::type_index, ComponentPermissions>& uniqueRegs);

    private:
        ECSCore* m_pECS;
        uint32 m_Phase;
        uint32 m_JobID;
    };
}
