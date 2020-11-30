#include "Rendering/EntityMaskManager.h"

#include "Utilities/StringUtilities.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Player/PlayerRelatedComponent.h"


namespace LambdaEngine
{
	bool EntityMaskManager::Init()
	{
		if (!s_Finalized)
		{
			BindTypeToExtensionDesc(MeshPaintComponent::Type(),		{ 0 }, false,	0x2);	// Bit = 0x2
			BindTypeToExtensionDesc(PlayerLocalComponent::Type(),	{ 0 }, true,	0x4);	// Bit = 0x4
			BindTypeToExtensionDesc(PlayerRelatedComponent::Type(),	{ 0 }, false,	0x8);	// Bit = 0x8
		}

		return true;
	}

	void EntityMaskManager::Finalize()
	{
		s_Finalized = true;
	}

	void LambdaEngine::EntityMaskManager::RemoveAllExtensionsFromEntity(Entity entity)
	{
		s_EntityToExtensionGroupEntryMap.erase(entity);
	}

	void EntityMaskManager::AddExtensionToEntity(Entity entity, const ComponentType* pType, const DrawArgExtensionData* pDrawArgExtension)
	{
		bool inverted;
		uint32 extensionFlag = GetExtensionFlag(pType, inverted);

		// Bind entity to the extension data
		auto it = s_EntityToExtensionGroupEntryMap.find(entity);
		if (it != s_EntityToExtensionGroupEntryMap.end())
		{
			DrawArgExtensionGroup& extensionGroup = it->second.ExtensionGroup;
			uint32 newIndex = extensionGroup.ExtensionCount++;

			if (pDrawArgExtension != nullptr)
			{
				CopyDrawArgExtensionData(extensionGroup.pExtensions[newIndex], pDrawArgExtension);
				extensionGroup.TotalTextureCount += pDrawArgExtension->TextureCount;
			}

			//extensionGroup.pExtensions[newIndex].ExtensionID = extensionFlag;

			extensionGroup.pExtensionFlags[newIndex] = extensionFlag;

			if (inverted)
			{
				it->second.Mask &= ~extensionFlag;
			}
			else
			{
				it->second.Mask |= extensionFlag;
			}

			return;
		}

		DrawArgExtensionGroupEntry& groupEntry	= s_EntityToExtensionGroupEntryMap[entity];
		DrawArgExtensionGroup& extensionGroup	= groupEntry.ExtensionGroup;
		extensionGroup.pExtensionFlags[0] = extensionFlag;
		extensionGroup.ExtensionCount = 1;

		if (pDrawArgExtension != nullptr)
		{
			CopyDrawArgExtensionData(extensionGroup.pExtensions[0], pDrawArgExtension);
			extensionGroup.TotalTextureCount = pDrawArgExtension->TextureCount;
		}

		//extensionGroup.pExtensions[0].ExtensionID = extensionFlag;
		groupEntry.Mask = s_DefaultMask;

		if (!inverted) 
			groupEntry.Mask |= extensionFlag;
	}

	DrawArgExtensionGroup* EntityMaskManager::GetExtensionGroup(Entity entity)
	{
		VALIDATE(s_EntityToExtensionGroupEntryMap.contains(entity));
		return &s_EntityToExtensionGroupEntryMap[entity].ExtensionGroup;
	}

	uint32 EntityMaskManager::FetchEntityMask(Entity entity)
	{
		auto it = s_EntityToExtensionGroupEntryMap.find(entity);
		if (it != s_EntityToExtensionGroupEntryMap.end())
			return it->second.Mask;
		return FetchDefaultEntityMask(); // No extra extension is used.
	}

	uint32 EntityMaskManager::FetchDefaultEntityMask()
	{
		return s_DefaultMask;
	}

	TArray<uint32> EntityMaskManager::ExtractComponentFlagsFromEntityMask(uint32 mask)
	{
		TArray<uint32> componentFlags;
		for (uint32 bit = 1; bit < sizeof(mask) * 8; bit++)
		{
			// Check if bit is set, if it is. That bit is the mask for the component.
			uint32 componentMask = mask & (1 << bit);
			if (componentMask > 0)
				componentFlags.PushBack(componentMask);
		}
		return componentFlags;
	}

	uint32 EntityMaskManager::GetExtensionFlag(const ComponentType* pType, bool& inverted)
	{
		if (auto it = s_ComponentTypeToMaskMap.find(pType); it != s_ComponentTypeToMaskMap.end())
		{
			inverted = it->second.Inverted;
			return it->second.Flag;
		}

		LOG_WARNING("[EntityMaskManager]: New flag required for Component type %s but EntityMaskManager is already intialized, returning default mask %x", pType->GetName(), s_DefaultMask);
		LOG_WARNING("[EntityMaskManager]: Generate a flag using BindTypeToExtensionDesc().");
		return s_DefaultMask;

	}

	const DrawArgExtensionDesc& EntityMaskManager::GetExtensionDescFromExtensionFlag(uint32 flag)
	{
		ASSERT(s_ExtensionMaskToExtensionDescMap.contains(flag));
		return s_ExtensionMaskToExtensionDescMap[flag];
	}

	void EntityMaskManager::BindTypeToExtensionDesc(const ComponentType* pType, DrawArgExtensionDesc extensionDesc, bool invertOnNewComponentType, uint32 bitmask)
	{
		// uint32 extensionFlag = GetExtensionFlag(pType, invertOnNewComponentType);
		s_ComponentTypeToMaskMap[pType] = { .Flag = bitmask, .Inverted = invertOnNewComponentType };
		if (invertOnNewComponentType)
		{
			s_DefaultMask |= bitmask;
		}

		// Set extension description for later use
		auto eIt = s_ExtensionMaskToExtensionDescMap.find(bitmask);
		if (eIt == s_ExtensionMaskToExtensionDescMap.end())
			s_ExtensionMaskToExtensionDescMap[bitmask] = extensionDesc;
	}

	void EntityMaskManager::CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData* pSrc)
	{
		memcpy(&dest, pSrc, sizeof(DrawArgExtensionData));
	}
}
