#include "ECS/System.h"

#include "ECS/ECSCore.h"
#include "ECS/EntityPublisher.h"

namespace LambdaEngine
{
    void System::EnqueueRegistration(const SystemRegistration& systemRegistration)
    {
        Job job = {
            .Function = [this] {
                Tick(ECSCore::GetInstance()->GetDeltaTime());
            },
            .Components = GetUniqueComponentAccesses(systemRegistration.SubscriberRegistration)
        };

        uint32 phase = systemRegistration.Phase;
        std::function<bool()> initFunction = [this, phase, job] {
            if (!InitSystem())
            {
                return false;
            }

            ECSCore::GetInstance()->GetDeltaTime();

            ScheduleRegularWork(job, phase);
            return true;
        };

        SubscribeToEntities(systemRegistration.SubscriberRegistration, initFunction);
    }

    ComponentHandler* System::GetComponentHandler(const std::type_index& handlerType)
    {
        return ECSCore::GetInstance()->GetEntityPublisher()->GetComponentHandler(handlerType);
    }
}
