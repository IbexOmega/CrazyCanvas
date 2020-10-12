#include "Rendering/EntityMaskManager.h"

#include "Utilities/StringUtilities.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

namespace LambdaEngine
{
	bool EntityMaskManager::Init()
	{
		BindTypeToExtensionDesc(MeshPaintComponent::Type(), { 1 }, false);	// Mask = 0x2
		BindTypeToExtensionDesc(PlayerLocalComponent::Type(), { 0 }, true);	// Mask = 0x4

		return true;
	}

	void EntityMaskManager::AddExtensionToEntity(Entity entity, const ComponentType* type, const DrawArgExtensionData& drawArgExtension)
	{
		bool inverted;
		uint32 extensionMask = GetExtensionMask(type, inverted);

		// Bind entity to the extension data
		auto it = s_EntityToExtensionGroupEntryMap.find(entity);
		if (it != s_EntityToExtensionGroupEntryMap.end())
		{
			DrawArgExtensionGroup& extensionGroup = it->second.extensionGroup;
			uint32 newIndex = extensionGroup.ExtensionCount++;
			CopyDrawArgExtensionData(extensionGroup.pExtensions[newIndex], drawArgExtension);
			extensionGroup.pExtensions[newIndex].ExtensionID = extensionMask;

			extensionGroup.pExtensionMasks[newIndex] = extensionMask;

			if (inverted)
			{
				it->second.Mask &= ~extensionMask;
			}
			else
			{
				it->second.Mask |= extensionMask;
			}

			return;
		}

		DrawArgExtensionGroupEntry& groupEntry = s_EntityToExtensionGroupEntryMap[entity];
		DrawArgExtensionGroup& extensionGroup = groupEntry.extensionGroup;
		extensionGroup.ExtensionCount = 1;
		extensionGroup.pExtensionMasks[0] = extensionMask;
		CopyDrawArgExtensionData(extensionGroup.pExtensions[0], drawArgExtension);
		extensionGroup.pExtensions[0].ExtensionID = extensionMask;
		groupEntry.Mask = 0x1;

		if (!inverted) groupEntry.Mask |= extensionMask;

		//Loop through all ExtensionMasks and set those which are inverted
		for (const std::pair<const ComponentType*, ComponentBit>& componentBitPair : s_ComponentTypeToMaskMap)
		{
			if (componentBitPair.second.Inverted && componentBitPair.first != type)
			{
				groupEntry.Mask |= componentBitPair.second.Bit;
			}
		}
	}

	DrawArgExtensionGroup& EntityMaskManager::GetExtensionGroup(Entity entity)
	{
		VALIDATE(s_EntityToExtensionGroupEntryMap.contains(entity));
		return s_EntityToExtensionGroupEntryMap[entity].extensionGroup;
	}

	uint32 EntityMaskManager::FetchEntityMask(Entity entity)
	{
		auto it = s_EntityToExtensionGroupEntryMap.find(entity);
		if (it != s_EntityToExtensionGroupEntryMap.end())
			return it->second.Mask;
		return 1; // No extra extension is used.
	}

	TArray<uint32> EntityMaskManager::ExtractComponentMasksFromEntityMask(uint32 mask)
	{
		TArray<uint32> componentMasks;
		for (uint32 bit = 1; bit < sizeof(mask) * 8; bit++)
		{
			// Check if bit is set, if it is. That bit is the mask for the component.
			uint32 componentMask = mask & (1 << bit);
			if (componentMask > 0)
				componentMasks.PushBack(componentMask);
		}
		return componentMasks;
	}

	uint32 EntityMaskManager::GetExtensionMask(const ComponentType* type, bool& inverted)
	{
		if (auto it = s_ComponentTypeToMaskMap.find(type); it != s_ComponentTypeToMaskMap.end())
		{
			inverted = it->second.Inverted;
			return it->second.Bit;
		}

		// Generate a mask for this component type. Mask 0 is used as an error code.
		static uint32 s_MaskCounter = 0;
		uint32 bit = BIT(++s_MaskCounter);

		//Set bit on other ComponentTypes
		s_ComponentTypeToMaskMap[type] = { .Bit = bit, .Inverted = inverted };

		return bit;
	}

	const DrawArgExtensionDesc& EntityMaskManager::GetExtensionDescFromExtensionMask(uint32 mask)
	{
		ASSERT(s_ExtensionMaskToExtensionDescMap.contains(mask));
		return s_ExtensionMaskToExtensionDescMap[mask];
	}

	void EntityMaskManager::BindTypeToExtensionDesc(const ComponentType* type, DrawArgExtensionDesc extensionDesc, bool invertOnNewComponentType)
	{
		uint32 extensionMask = GetExtensionMask(type, invertOnNewComponentType);

		// Set extension description for later use
		auto eIt = s_ExtensionMaskToExtensionDescMap.find(extensionMask);
		if (eIt == s_ExtensionMaskToExtensionDescMap.end())
			s_ExtensionMaskToExtensionDescMap[extensionMask] = extensionDesc;
	}

	void EntityMaskManager::CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData& src)
	{
		memcpy(&dest, &src, sizeof(DrawArgExtensionData));
	}
}
