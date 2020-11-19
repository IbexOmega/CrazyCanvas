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
		m_DeletedEntities.clear();
	}

	void InheritanceComponentOwner::ParentComponentDestructor(ParentComponent& parentComponent, Entity entity)
	{
		if (parentComponent.DeleteParentOnRemoval)
		{
			ECSCore* pECS = ECSCore::GetInstance();

			if (m_DeletedEntities.insert(parentComponent.Parent).second)
			{
				pECS->RemoveEntity(parentComponent.Parent);
			}
		}
	}

	void InheritanceComponentOwner::ChildComponentDestructor(ChildComponent& childComponent, Entity entity)
	{
		ECSCore* pECS = ECSCore::GetInstance();
		TArray<Entity> childEntities = childComponent.GetEntities();
		TArray<bool> chilrenDeletionProperties = childComponent.GetDeletionProperties();

		for (uint32 i = 0; i < chilrenDeletionProperties.GetSize(); i++)
		{
			if (chilrenDeletionProperties[i])
			{
				Entity childEntity = childEntities[i];

				if (m_DeletedEntities.insert(childEntity).second)
				{
					pECS->RemoveEntity(childEntity);
				}
			}
		}
	}
}