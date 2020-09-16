#include "ECS/ComponentHandler.h"

#include "ECS/ECSCore.h"

namespace LambdaEngine
{
    ComponentHandler::~ComponentHandler()
    {
        ECSCore::GetInstance()->GetEntityPublisher()->DeregisterComponentHandler(this);
    }

    void ComponentHandler::RegisterHandler(const ComponentHandlerRegistration& handlerRegistration)
    {
        // Write handled types
        m_HandledTypes.Reserve(handlerRegistration.ComponentRegistrations.GetSize());

        for (const ComponentRegistration& componentRegistration : handlerRegistration.ComponentRegistrations) {
            m_HandledTypes.PushBack(componentRegistration.TID);
        }

        ECSCore::GetInstance()->EnqueueComponentHandlerRegistration(handlerRegistration);
    }

    void ComponentHandler::RegisterComponent(Entity entity, std::type_index componentType)
    {
        ECSCore::GetInstance()->ComponentAdded(entity, componentType);
    }
}
