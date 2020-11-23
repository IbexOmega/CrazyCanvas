#pragma once

#include "ECS/Component.h"
#include "ECS/Entity.h"

namespace LambdaEngine
{
	constexpr const uint32 MAX_CHILD_COMPONENTS = 8;

	struct ParentComponent
	{
		DECL_COMPONENT(ParentComponent);
		Entity	Parent;
		bool	Attached;
		bool	DeleteParentOnRemoval = false;
	};

	struct ChildComponent
	{
		DECL_COMPONENT(ParentComponent);

	public:
		inline void AddChild(const String& tag, Entity entity, bool deleteOnRemoval)
		{
			int32 index = GetIndex(tag);
			if (index != -1)
			{
				LOG_WARNING("[ChildComponent]: Tag '%s' already used", tag.c_str());
				return;
			}

			Tags.EmplaceBack(tag);
			Children.EmplaceBack(entity);
			DeleteChildrenOnRemoval.EmplaceBack(deleteOnRemoval);
		}

		inline void RemoveChild(const String& tag)
		{
			int32 index = GetIndex(tag);
			if (index != -1)
			{
				Tags.Erase(Tags.Begin() + index);
				Children.Erase(Children.Begin() + index);
				DeleteChildrenOnRemoval.Erase(DeleteChildrenOnRemoval.Begin() + index);
			}
		}

		inline Entity GetEntityWithTag(const String& tag) const
		{
			int32 index = GetIndex(tag);
			if (index != -1)
				return Children[index];
			else
				return UINT32_MAX;
		}

		inline const TArray<String>& GetTags() const
		{
			return Tags;
		}

		inline const TArray<Entity>& GetEntities() const
		{
			return Children;
		}

		inline const TArray<bool>& GetDeletionProperties() const
		{
			return DeleteChildrenOnRemoval;
		}

	private:
		inline int32 GetIndex(const String& tag) const
		{
			for (uint32 i = 0; i < Tags.GetSize(); i++)
			{
				if (Tags[i] == tag)
				{
					return i;
				}
			}
			return -1;
		}

	private:
		TArray<String> Tags;
		TArray<Entity> Children;
		TArray<bool> DeleteChildrenOnRemoval;
	};
}