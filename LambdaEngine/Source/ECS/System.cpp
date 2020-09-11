#include "ECS/System.h"

#include "ECS/ECSCore.h"
#include "ECS/EntityPublisher.h"

namespace LambdaEngine
{
    System::System(ECSCore* pECS)
        :EntitySubscriber(pECS),
        RegularWorker(pECS),
        m_pECS(pECS),
        m_SystemID(UINT32_MAX)
    {}

    void System::EnqueueRegistration(const SystemRegistration& systemRegistration)
    {
        Job job = {
            .Function = [this] {
                Tick(m_pECS->GetDeltaTime());
            },
            .Components = GetUniqueComponentAccesses(systemRegistration.SubscriberRegistration)
        };

        uint32 phase = systemRegistration.Phase;
        std::function<bool()> initFunction = [this, phase, job] {
            if (!InitSystem())
            {
                return false;
            }

            m_pECS->GetDeltaTime();

            ScheduleRegularWork(job, phase);
            return true;
        };

        SubscribeToEntities(systemRegistration.SubscriberRegistration, initFunction);
    }

    ComponentHandler* System::GetComponentHandler(const std::type_index& handlerType)
    {
        return m_pECS->GetEntityPublisher()->GetComponentHandler(handlerType);
    }
}
