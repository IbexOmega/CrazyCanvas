#pragma once

#include "Defines.h"
#include "ECS/ComponentType.h"
#include "ECS/Entity.h"

#include "Rendering/RenderGraphTypes.h"

namespace LambdaEngine
{
	class Texture;
	class TextureView;
	class Buffer;

	typedef uint32 FDrawArgExtensionFlags;
	enum FDrawArgExtensionFlag : FDrawArgExtensionFlags
	{
		DRAW_ARG_EXTENSION_TYPE_NONE		= 0,
		DRAW_ARG_EXTENSION_TYPE_TEXTURE_2D	= FLAG(1),
		DRAW_ARG_EXTENSION_TYPE_BUFFER		= FLAG(2)
	};

	struct DrawArgExtensionGroupEntry
	{
		uint32					Mask = 0;
		DrawArgExtensionGroup	extensionGroup;
	};

	class EntityMaskManager
	{
	public:
		DECL_STATIC_CLASS(EntityMaskManager);

		static void AddExtensionToEntity(Entity entity, const ComponentType* type, const DrawArgExtensionData& DrawArgExtension);
		static DrawArgExtensionGroup& GetExtensionGroup(Entity entity);

		static uint32 FetchEntityMask(Entity entity);

		static TArray<uint32> ExtractComponentMasksFromEntityMask(uint32 mask);

	private:
		static uint32 GetExtensionMask(const ComponentType* type);

		static void CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData& src);

		static THashTable<const ComponentType*, uint32>			s_ComponentTypeToMaskMap;
		static THashTable<Entity, DrawArgExtensionGroupEntry>	s_EntityToExtensionGroupEntryMap;
	};
}