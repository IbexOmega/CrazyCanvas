#include "ECS/ECSBooter.h"

#include "ECS/ComponentHandler.h"
#include "ECS/ECSCore.h"
#include "ECS/System.h"
#include "Log/Log.h"

namespace LambdaEngine
{
    ECSBooter::ECSBooter(ECSCore* pECS)
        :m_pECS(pECS)
    {}

    void ECSBooter::EnqueueComponentHandlerRegistration(const ComponentHandlerRegistration& handlerRegistration)
    {
        m_ComponentHandlersToRegister.EmplaceBack(handlerRegistration);
    }

    void ECSBooter::EnqueueSubscriberInitialization(const Subscriber& subscriber)
    {
        m_Subscribers.EmplaceBack(subscriber);
    }

    void ECSBooter::PerformBootups()
    {
        if (m_ComponentHandlersToRegister.IsEmpty() && m_Subscribers.IsEmpty())
        {
            return;
        }

        BootHandlers();
        BootSubscribers();
    }

    void ECSBooter::BootHandlers()
    {
        BuildBootInfos();
        FinalizeHandlerBootDependencies();

        auto currentHandlerItr = m_HandlersBootInfos.begin();

        // Contains iterators to the current handler's dependencies
        TArray<THashTable<std::type_index, ComponentHandlerBootInfo>::iterator> openList;
        openList.Reserve((uint32)m_HandlersBootInfos.size());

        // Outer loop: goes through handlers in m_HandlersToBoot until each handler has been booted
        while (currentHandlerItr != m_HandlersBootInfos.end())
        {
            // Inner loop: recursively goes through the current handler's dependencies until it is booted
            auto dependencyItr = currentHandlerItr;

            while (!currentHandlerItr->second.InClosedList)
            {
                ComponentHandlerBootInfo& bootInfo = dependencyItr->second;
                if (bootInfo.InOpenList)
                {
                    if (dependencyItr != openList.GetBack())
                    {
                        LOG_ERROR("Detected cyclic dependencies between component handlers, can not boot: %s", currentHandlerItr->first.name());
                        break;
                    }
                } else
                {
                    openList.PushBack(dependencyItr);
                    bootInfo.InOpenList = true;
                }

                // Check if the handler has any unbooted dependencies
                auto unbootedDependencyItr = std::find_if(bootInfo.DependencyMapIterators.begin(), bootInfo.DependencyMapIterators.end(),
                    [](auto dependencyItr) -> bool { return !dependencyItr->second.InClosedList; });

                if (unbootedDependencyItr == bootInfo.DependencyMapIterators.end())
                {
                    // All dependencies are booted, boot the handler
                    BootHandler(bootInfo);
                    openList.PopBack();

                    if (!openList.IsEmpty())
                    {
                        dependencyItr = openList.GetBack();
                    }
                }
                else
                {
                    // Not all dependencies are booted, try to boot a dependency
                    dependencyItr = *unbootedDependencyItr;
                }
            }

            currentHandlerItr++;
            openList.Clear();
        }

        m_ComponentHandlersToRegister.Clear();
        m_HandlersBootInfos.clear();
    }

    void ECSBooter::BootSubscribers()
    {
        EntityPublisher* pEntityPublisher = m_pECS->GetEntityPublisher();

        for (const Subscriber& subscriber : m_Subscribers)
        {
            if (!subscriber.InitFunction())
            {
                LOG_ERROR("Failed to initialize subscriber");
            } else
            {
                *subscriber.pSubscriberID = pEntityPublisher->SubscribeToComponents(subscriber.ComponentSubscriptions);
            }
        }

        m_Subscribers.Clear();
    }

    void ECSBooter::BuildBootInfos()
    {
        for (const ComponentHandlerRegistration& handlerReg : m_ComponentHandlersToRegister)
        {
            ComponentHandlerBootInfo bootInfo = {};
            bootInfo.pHandlerRegistration = &handlerReg;
            bootInfo.DependencyMapIterators.Reserve(handlerReg.HandlerDependencies.GetSize());
            bootInfo.InOpenList = false;
            bootInfo.InClosedList = false;

            // Map the handler's type index to its boot info
            m_HandlersBootInfos[handlerReg.pComponentHandler->GetHandlerType()] = bootInfo;
        }
    }

    void ECSBooter::FinalizeHandlerBootDependencies()
    {
        auto bootInfoItr = m_HandlersBootInfos.begin();
        while (bootInfoItr != m_HandlersBootInfos.end())
        {
            ComponentHandlerBootInfo& bootInfo = bootInfoItr->second;
            bool hasDependencies = true;

            for (const std::type_index dependencyTID : bootInfo.pHandlerRegistration->HandlerDependencies)
            {
                // Find the dependency's index
                auto dependencyItr = m_HandlersBootInfos.find(dependencyTID);
                if (dependencyItr != m_HandlersBootInfos.end())
                {
                    // The dependency is enqueued for bootup, store its iterator
                    bootInfo.DependencyMapIterators.PushBack(dependencyItr);
                } else
                {
                    // The dependency is not enqueued for bootup, it might already be booted
                    if (!m_pECS->GetEntityPublisher()->GetComponentHandler(dependencyTID))
                    {
                        LOG_ERROR("Cannot boot handler: %s, missing dependency: %s", bootInfo.pHandlerRegistration->pComponentHandler->GetHandlerType().name(), dependencyTID.name());
                        hasDependencies = false;
                        break;
                    }
                }
            }

            if (hasDependencies) {
                bootInfoItr++;
            } else {
                bootInfoItr = m_HandlersBootInfos.erase(bootInfoItr);
            }
        }
    }

    void ECSBooter::BootHandler(ComponentHandlerBootInfo& bootInfo)
    {
        bootInfo.InOpenList = false;
        bootInfo.InClosedList = true;

        if (!bootInfo.pHandlerRegistration->pComponentHandler->InitHandler())
        {
            LOG_ERROR("Failed to initialize component handler: %s", bootInfo.pHandlerRegistration->pComponentHandler->GetHandlerType().name());
        }
        else
        {
            m_pECS->GetEntityPublisher()->RegisterComponentHandler(*bootInfo.pHandlerRegistration);
        }
    }
}
