#pragma once

#include "Containers/THashTable.h"
#include "ECS/EntitySubscriber.h"

#include <typeindex>
#include <queue>
#include <vector>

namespace LambdaEngine
{
    class ComponentHandler;
    class ECSCore;
    class JobScheduler;
    struct ComponentHandlerRegistration;
    struct RendererRegistration;
    struct SystemRegistration;

    struct ComponentHandlerBootInfo
    {
        const ComponentHandlerRegistration* pHandlerRegistration;
        // Iterators that point out a handler's place in the boot info map
        TArray<THashTable<std::type_index, ComponentHandlerBootInfo>::iterator> DependencyMapIterators;
        // Used during bootup, to detect cyclic dependencies
        bool InOpenList;
        bool InClosedList;
    };

    struct Subscriber
    {
        EntitySubscriberRegistration ComponentSubscriptions;
        std::function<bool()> InitFunction;
        uint32* pSubscriberID;
    };

    class ECSBooter
    {
    public:
        ECSBooter(ECSCore* pECS);
        ~ECSBooter() = default;

        void EnqueueComponentHandlerRegistration(const ComponentHandlerRegistration& handlerRegistration);
        void EnqueueSubscriberInitialization(const Subscriber& subscriber);

        void PerformBootups();

    private:
        struct OpenListInfo {
            ComponentHandlerBootInfo& bootInfo;
            bool inOpenList, inClosedList;
        };

    private:
        void BootHandlers();
        void BootSubscribers();

        void BuildBootInfos();
        // Get the map iterators of each handler's dependency, for faster lookups. Also remove dependencies that have already been booted.
        void FinalizeHandlerBootDependencies();
        void BootHandler(ComponentHandlerBootInfo& bootInfo);

    private:
        ECSCore* m_pECS;

        TArray<ComponentHandlerRegistration> m_ComponentHandlersToRegister;
        TArray<Subscriber> m_Subscribers;

        // Maps handlers' types to their boot info
        THashTable<std::type_index, ComponentHandlerBootInfo> m_HandlersBootInfos;
    };
}
