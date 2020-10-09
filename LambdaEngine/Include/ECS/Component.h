#pragma once

#include "Containers/TArray.h"
#include "Containers/String.h"
#include "ECS/ComponentType.h"

#define DECL_COMPONENT(Component) \
	private: \
		inline static constexpr const ComponentType s_Type = ComponentType(#Component); \
	public: \
		FORCEINLINE static const ComponentType* Type() \
		{ \
			return &s_Type; \
		} \
		static constexpr bool HasDirtyFlag() \
		{ \
			return false; \
		} \

#define DECL_COMPONENT_WITH_DIRTY_FLAG(Component) \
	protected: \
		inline static constexpr const ComponentType s_Type = ComponentType(#Component); \
	public: \
		FORCEINLINE static const ComponentType* Type() \
		{ \
			return &s_Type; \
		} \
		static constexpr bool HasDirtyFlag() \
		{ \
			return true; \
		} \
		bool Dirty = true \

namespace LambdaEngine
{
	enum ComponentPermissions
	{
		NDA = 0,	// No Data Access
		R   = 1,	// Read
		RW  = 2		// Read & Write
	};

	template <typename Comp>
	class GroupedComponent
	{
	public:
		ComponentPermissions Permissions = NDA; // The default permissions for any component type in a component group
	};

	struct ComponentAccess
	{
		template <typename Comp>
		ComponentAccess(GroupedComponent<Comp> groupedComponent)
			:
			Permissions(groupedComponent.Permissions),
			pTID(Comp::Type())
		{}

		ComponentAccess(ComponentPermissions permissions, const ComponentType* pType)
			:
			Permissions(permissions),
			pTID(pType)
		{}

		ComponentPermissions Permissions;
		const ComponentType* pTID;
	};

	class IComponentGroup
	{
	public:
		virtual TArray<ComponentAccess> ToArray() const = 0;
	};

	template <typename Comp>
	struct ComponentOwnership
	{
		// Called just before deleting a component
		std::function<void(Comp&)> Destructor;
		/**
		 * Serialize the component into the buffer, excluding the type hash.
		 * \return The required size of the serialization in bytes.
		 * Does not write to the buffer if said size is greater than the provided bufferSize.
		*/
		std::function<uint32(const Comp& component, uint8* pBuffer, uint32 bufferSize)> Serialize;
		/**
		 * Deserialize the buffer into the referenced component. Does not add or register the component.
		 * The passed component could be referencing an already existing component. One should be aware of this to
		 * to avoid allocating memory for members of the component without deleting previous allocations.
		 * \return Success or failure.
		*/
		std::function<bool(Comp& component, uint32 serializationSize, const uint8* pBuffer)> Deserialize;
	};
}
