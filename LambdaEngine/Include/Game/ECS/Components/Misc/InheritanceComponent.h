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
	};

	struct StepParentComponent
	{
		DECL_COMPONENT(StepParentComponent);
		Entity	Owner;
	};

	struct ChildComponent
	{
		DECL_COMPONENT(ParentComponent);

	public:
		inline void AddChild(Entity entity, const String& tag)
		{
			int32 index = GetIndex(tag);
			if (index != -1)
			{
				LOG_WARNING("[ChildComponent]: Tag '%s' already used", tag.c_str());
				return;
			}

			Children.EmplaceBack(entity);
			Tags.EmplaceBack(tag);
		}

		inline void RemoveChild(const String& tag)
		{
			int32 index = GetIndex(tag);
			if (index != -1)
			{
				Children.Erase(Children.Begin() + index);
				Tags.Erase(Tags.Begin() + index);
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
		TArray<Entity> Children;
		TArray<String> Tags;
	};
}