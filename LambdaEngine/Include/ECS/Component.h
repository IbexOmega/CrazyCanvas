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

	struct ComponentAccess
	{
		ComponentPermissions Permissions;
		const ComponentType* TID;
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
	};
}
