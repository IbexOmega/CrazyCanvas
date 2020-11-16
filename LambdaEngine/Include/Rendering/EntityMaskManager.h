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
		DrawArgExtensionGroup	ExtensionGroup;
	};

	class EntityMaskManager
	{
		struct ComponentBit
		{
			uint32		Flag		= 0x0;
			bool		Inverted	= false; //Inverted means that the Bit will be set if the Component which maps to this ComponentBit is not present in an Entity
		};

	public:
		DECL_STATIC_CLASS(EntityMaskManager);

		static bool Init();
		static void Finalize();

		static void RemoveAllExtensionsFromEntity(Entity entity);
		static void AddExtensionToEntity(Entity entity, const ComponentType* pType, const DrawArgExtensionData* pDrawArgExtension);
		static DrawArgExtensionGroup* GetExtensionGroup(Entity entity);

		static uint32 FetchEntityMask(Entity entity);
		static uint32 FetchDefaultEntityMask();

		static TArray<uint32> ExtractComponentFlagsFromEntityMask(uint32 mask);

		static uint32 GetExtensionFlag(const ComponentType* pType, bool& inverted);

		static const DrawArgExtensionDesc& GetExtensionDescFromExtensionFlag(uint32 flag);

		static void BindTypeToExtensionDesc(const ComponentType* pType, DrawArgExtensionDesc extensionDesc, bool invertOnNewComponentType);

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