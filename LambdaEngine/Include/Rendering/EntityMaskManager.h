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
		struct ComponentBit
		{
			uint32		Bit			= 0x0;
			bool		Inverted	= false; //Inverted means that the Bit will be set if the Component which maps to this ComponentBit is not present in an Entity
		};

	public:
		DECL_STATIC_CLASS(EntityMaskManager);

		static bool Init();
		static void Finalize();

		static void RemoveAllExtensionsFromEntity(Entity entity);
		static void AddExtensionToEntity(Entity entity, const ComponentType* type, const DrawArgExtensionData* pDrawArgExtension);
		static DrawArgExtensionGroup& GetExtensionGroup(Entity entity);

		static uint32 FetchEntityMask(Entity entity);
		static uint32 FetchDefaultEntityMask();

		static TArray<uint32> ExtractComponentMasksFromEntityMask(uint32 mask);

		static uint32 GetExtensionMask(const ComponentType* type, bool& inverted);

		static const DrawArgExtensionDesc& GetExtensionDescFromExtensionMask(uint32 mask);

		static void BindTypeToExtensionDesc(const ComponentType* type, DrawArgExtensionDesc extensionDesc, bool invertOnNewComponentType);

	private:
		static void CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData* pSrc);

	private:
		inline static THashTable<const ComponentType*, ComponentBit>	s_ComponentTypeToMaskMap;
		inline static THashTable<Entity, DrawArgExtensionGroupEntry>	s_EntityToExtensionGroupEntryMap;
		inline static THashTable<uint32, DrawArgExtensionDesc>			s_ExtensionMaskToExtensionDescMap;
		inline static uint32 s_DefaultMask = 1;
		inline static uint32 s_Finalized = false;
	};
}