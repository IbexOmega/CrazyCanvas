#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/RegularWorker.h"

#include "Time/API/Timestamp.h"

#include <functional>
#include <typeindex>

namespace LambdaEngine
{
    struct SystemRegistration
    {
        EntitySubscriberRegistration SubscriberRegistration;
        uint32_t Phase = 0;
    };

    class ComponentHandler;

    // A system processes components each frame in the tick function
    class System : private EntitySubscriber, private RegularWorker
    {
    public:
        // Registers the system in the system handler
        System() = default;

        // Deregisters system
        virtual ~System() = default;

        virtual void Tick(Timestamp deltaTime) = 0;

    protected:
        void RegisterSystem(const SystemRegistration& systemRegistration);
    };
}
