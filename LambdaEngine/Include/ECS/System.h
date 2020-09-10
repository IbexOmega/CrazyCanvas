#pragma once

#include "Containers/IDVector.h"
#include "ECS/Entity.h"
#include "ECS/EntitySubscriber.h"
#include "ECS/RegularWorker.h"

#include <functional>
#include <typeindex>
#include <vector>

namespace LambdaEngine
{
    struct SystemRegistration
    {
        EntitySubscriberRegistration SubscriberRegistration;
        uint32_t Phase = 0;
    };

    class ComponentHandler;
    class ECSCore;

    // A system processes components each frame in the tick function
    class System : private EntitySubscriber, private RegularWorker
    {
    public:
        // Registers the system in the system handler
        System(ECSCore* pECS);

        // Deregisters system
        virtual ~System() = default;

        virtual bool InitSystem() = 0;

        virtual void Tick(float dt) = 0;

        uint32 GetSystemID() const          { return m_SystemID; }
        void SetSystemID(uint32 systemID)   { m_SystemID = systemID; }

    protected:
        void EnqueueRegistration(const SystemRegistration& sysReg);

        ComponentHandler* GetComponentHandler(const std::type_index& handlerType);

    private:
        ECSCore* m_pECS;
        uint32 m_SystemID;
    };
}
