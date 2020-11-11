#include "Rendering/EntityMaskManager.h"

#include "Utilities/StringUtilities.h"
#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

namespace LambdaEngine
{
	bool EntityMaskManager::Init()
	{
		if (!s_Initialized)
		{
			BindTypeToExtensionDesc(MeshPaintComponent::Type(),	{ 1 }, false);	// Bit = 0x2
			BindTypeToExtensionDesc(PlayerLocalComponent::Type(), { 0 }, true);	// Bit = 0x4
			BindTypeToExtensionDesc(PlayerBaseComponent::Type(), { 0 }, false);	// Bit = 0x8

			s_Initialized = true;
		}
		return true;
	}

	void LambdaEngine::EntityMaskManager::RemoveAllExtensionsFromEntity(Entity entity)
	{
		s_EntityToExtensionGroupEntryMap.erase(entity);
	}

	void EntityMaskManager::AddExtensionToEntity(Entity entity, const ComponentType* type, const DrawArgExtensionData* pDrawArgExtension)
	{
		bool inverted;
		uint32 extensionMask = GetExtensionMask(type, inverted);

		// Bind entity to the extension data
		auto it = s_EntityToExtensionGroupEntryMap.find(entity);
		if (it != s_EntityToExtensionGroupEntryMap.end())
		{
			DrawArgExtensionGroup& extensionGroup = it->second.extensionGroup;
			uint32 newIndex = extensionGroup.ExtensionCount++;

			if (pDrawArgExtension != nullptr)
			{
				CopyDrawArgExtensionData(extensionGroup.pExtensions[newIndex], pDrawArgExtension);
			}

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

		if (pDrawArgExtension != nullptr)
		{
			CopyDrawArgExtensionData(extensionGroup.pExtensions[0], pDrawArgExtension);
		}

		extensionGroup.pExtensions[0].ExtensionID = extensionMask;
		groupEntry.Mask = s_DefaultMask;

		if (!inverted) 
			groupEntry.Mask |= extensionMask;
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
		return FetchDefaultEntityMask(); // No extra extension is used.
	}

	uint32 EntityMaskManager::FetchDefaultEntityMask()
	{
		return s_DefaultMask;
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

		if (!s_Initialized)
		{
			// Generate a mask for this component type. Mask 0 is used as an error code.
			static uint32 s_MaskCounter = 0;
			uint32 bit = BIT(++s_MaskCounter);

			//Set bit on other ComponentTypes
			s_ComponentTypeToMaskMap[type] = { .Bit = bit, .Inverted = inverted };

			if (inverted)
			{
				s_DefaultMask |= bit;
			}

			return bit;
		}
		else
		{
			LOG_WARNING("[EntityMaskManager]: New bit required for Component type %s but EntityMaskManager is already intialized, returning default mask %x", type->GetName(), s_DefaultMask);
			return s_DefaultMask;
		}
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

	void EntityMaskManager::CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData* pSrc)
	{
		memcpy(&dest, pSrc, sizeof(DrawArgExtensionData));
	}
}
