#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/RegularWorker.h"

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

        virtual bool InitSystem() = 0;

        virtual void Tick(float dt) = 0;

        uint32 GetSystemID() const          { return m_SystemID; }
        void SetSystemID(uint32 systemID)   { m_SystemID = systemID; }

    protected:
        void EnqueueRegistration(const SystemRegistration& systemRegistration);

        ComponentHandler* GetComponentHandler(const std::type_index& handlerType);

    private:
        uint32 m_SystemID = UINT32_MAX;
    };
}
