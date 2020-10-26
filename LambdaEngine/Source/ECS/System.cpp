#include "ECS/System.h"

#include "ECS/ECSCore.h"
#include "ECS/EntityPublisher.h"

namespace LambdaEngine
{
    void System::RegisterSystem(SystemRegistration& systemRegistration)
    {
        RegularWorkInfo regularWorkInfo =
        {
            .TickFunction = std::bind_front(&System::Tick, this),
            .EntitySubscriberRegistration = systemRegistration.SubscriberRegistration,
            .Phase = systemRegistration.Phase,
            .TickPeriod = systemRegistration.TickFrequency == 0 ? 0.0f : 1.0f / systemRegistration.TickFrequency
        };

        SubscribeToEntities(systemRegistration.SubscriberRegistration);
        ScheduleRegularWork(regularWorkInfo);
    }
}
