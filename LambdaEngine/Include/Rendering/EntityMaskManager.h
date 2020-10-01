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

	struct DrawArgExtensionGroupEntry
	{
		uint32					Mask = 0;
		DrawArgExtensionGroup	extensionGroup;
	};

	class EntityMaskManager
	{
	public:
		DECL_STATIC_CLASS(EntityMaskManager);

		static bool Init();

		static void AddExtensionToEntity(Entity entity, const ComponentType* type, const DrawArgExtensionData& DrawArgExtension);
		static DrawArgExtensionGroup& GetExtensionGroup(Entity entity);

		static uint32 FetchEntityMask(Entity entity);

		static TArray<uint32> ExtractComponentMasksFromEntityMask(uint32 mask);

		static uint32 GetExtensionMask(const ComponentType* type);

		static const DrawArgExtensionDesc& GetExtensionDescFromExtensionMask(uint32 mask);

	private:
		static void BindTypeToExtensionDesc(const ComponentType* type, DrawArgExtensionDesc extensionDesc);

		static void CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData& src);

		static THashTable<const ComponentType*, uint32>			s_ComponentTypeToMaskMap;
		static THashTable<Entity, DrawArgExtensionGroupEntry>	s_EntityToExtensionGroupEntryMap;
		static THashTable<uint32, DrawArgExtensionDesc>			s_ExtensionMaskToExtensionDescMap;
	};
}