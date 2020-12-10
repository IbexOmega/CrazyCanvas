#include "Game/ECS/ComponentOwners/Misc/InheritanceComponentOwner.h"

namespace LambdaEngine
{
	InheritanceComponentOwner InheritanceComponentOwner::s_Instance;

	bool InheritanceComponentOwner::Init()
	{
		SetComponentOwner<ParentComponent>({ .Destructor = std::bind_front(&InheritanceComponentOwner::ParentComponentDestructor, this) });
		SetComponentOwner<ChildComponent>({ .Destructor = std::bind_front(&InheritanceComponentOwner::ChildComponentDestructor, this) });
		return true;
	}

	bool InheritanceComponentOwner::Release()
	{
		ECSCore* pECS = ECSCore::GetInstance();
		pECS->UnsetComponentOwner(ParentComponent::Type());
		pECS->UnsetComponentOwner(ChildComponent::Type());
		return true;
	}

	void InheritanceComponentOwner::Tick()
	{
		ECSCore* pECS = ECSCore::GetInstance();

		for (Entity entity : m_EntitiesToDelete)
		{
			pECS->RemoveEntity(entity);
		}

		m_EntitiesToDelete.clear();
	}

	void InheritanceComponentOwner::ParentComponentDestructor(ParentComponent& parentComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		if (parentComponent.DeleteParentOnRemoval)
		{
			m_EntitiesToDelete.insert(parentComponent.Parent);
		}
	}

	void InheritanceComponentOwner::ChildComponentDestructor(ChildComponent& childComponent, Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);
	
		TArray<Entity> childEntities = childComponent.GetEntities();
		TArray<bool> chilrenDeletionProperties = childComponent.GetDeletionProperties();

		for (uint32 i = 0; i < chilrenDeletionProperties.GetSize(); i++)
		{
			if (chilrenDeletionProperties[i])
			{
				Entity childEntity = childEntities[i];
				m_EntitiesToDelete.insert(childEntity);
			}
		}
	}
}