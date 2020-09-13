#pragma once

#include "Containers/TArray.h"
#include "ECS/Component.h"
#include "ECS/Entity.h"

#include <functional>

namespace LambdaEngine
{
    class ECSCore;
    class IDContainer;

    struct ComponentRegistration
    {
        std::type_index TID;
        IDContainer* pComponentContainer;
        std::function<void(Entity)> ComponentDestructor;
    };

    class ComponentHandler;

    struct ComponentHandlerRegistration
    {
        ComponentHandler* pComponentHandler;
        TArray<ComponentRegistration> ComponentRegistrations;
        TArray<std::type_index> HandlerDependencies;
    };

    /*
        A component handler stores components and has functions for creating components and
        performing tasks on them (eg. perform transformations on transform components)
    */
    class ComponentHandler
    {
    public:
        ComponentHandler(std::type_index tid_handler);
        // Deregisters component handler and deletes components
        virtual ~ComponentHandler();

        virtual bool InitHandler() = 0;

        const TArray<std::type_index>& GetHandledTypes() const;
        std::type_index GetHandlerType() const;

    protected:
        /**
         * Registers the component handler's type of components it handles
         * @param componentQueries Functions for asking if an entity has a component of a certain type
         */
        void RegisterHandler(const ComponentHandlerRegistration& handlerRegistration);

        // Tell the system subscriber a component has been created
        void RegisterComponent(Entity entity, std::type_index componentType);

    protected:
        TArray<std::type_index> m_HandledTypes;

    private:
        std::type_index m_TID;
    };
}
