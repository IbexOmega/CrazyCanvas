#include "ECS/System.h"

#include "ECS/ECSCore.h"
#include "ECS/EntityPublisher.h"

namespace LambdaEngine
{
    void System::RegisterSystem(SystemRegistration& systemRegistration)
    {
        Job job = {
            .Function = [this] {
                Tick(ECSCore::GetInstance()->GetDeltaTime());
            },
            .Components = GetUniqueComponentAccesses(systemRegistration.SubscriberRegistration)
        };

        SubscribeToEntities(systemRegistration.SubscriberRegistration);
        ScheduleRegularWork(job, systemRegistration.Phase);
    }
}
