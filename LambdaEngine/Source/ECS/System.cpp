#include "ECS/System.h"

#include "ECS/ECSCore.h"
#include "ECS/EntityPublisher.h"

namespace LambdaEngine
{
    System::~System()
    {
        ECSCore* pECS = ECSCore::GetInstance();
        const uint32 jobID = GetJobID();
        if (pECS && jobID != UINT32_MAX)
        {
            pECS->DeregisterSystem(jobID);
        }
    }

    void System::RegisterSystem(const String& systemName, SystemRegistration& systemRegistration)
    {
        m_SystemName = systemName;

        const RegularWorkInfo regularWorkInfo =
        {
            .TickFunction = std::bind_front(&System::Tick, this),
            .EntitySubscriberRegistration = systemRegistration.SubscriberRegistration,
            .Phase = systemRegistration.Phase,
            .TickPeriod = systemRegistration.TickFrequency == 0 ? 0.0f : 1.0f / systemRegistration.TickFrequency
        };

        SubscribeToEntities(systemRegistration.SubscriberRegistration);
        ScheduleRegularWork(regularWorkInfo);
        ECSCore::GetInstance()->RegisterSystem(this, GetJobID());
    }
}
