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
		uint32 extensionFlag = GetExtensionFlag(type, inverted);

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

			//extensionGroup.pExtensions[newIndex].ExtensionID = extensionFlag;

			//extensionGroup.pExtensionFlags[newIndex] = extensionFlag;

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

		DrawArgExtensionGroupEntry& groupEntry = s_EntityToExtensionGroupEntryMap[entity];
		DrawArgExtensionGroup& extensionGroup = groupEntry.extensionGroup;
		extensionGroup.ExtensionCount = 1;
		//extensionGroup.pExtensionFlags[0] = extensionFlag;

		if (pDrawArgExtension != nullptr)
		{
			CopyDrawArgExtensionData(extensionGroup.pExtensions[0], pDrawArgExtension);
		}

		//extensionGroup.pExtensions[0].ExtensionID = extensionFlag;
		groupEntry.Mask = s_DefaultMask;

		if (!inverted) groupEntry.Mask |= extensionFlag;
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

	uint32 EntityMaskManager::GetExtensionFlag(const ComponentType* type, bool& inverted)
	{
		if (auto it = s_ComponentTypeToMaskMap.find(type); it != s_ComponentTypeToMaskMap.end())
		{
			inverted = it->second.Inverted;
			return it->second.Flag;
		}

		if (!s_Initialized)
		{
			// Generate a mask for this component type. Mask 0 is used as an error code.
			static uint32 s_BitCounter = 0;
			uint32 flag = BIT(++s_BitCounter);

			//Set bit on other ComponentTypes
			s_ComponentTypeToMaskMap[type] = { .Flag = flag, .Inverted = inverted };

			if (inverted)
			{
				s_DefaultMask |= flag;
			}

			return flag;
		}
		else
		{
			LOG_WARNING("[EntityMaskManager]: New flag required for Component type %s but EntityMaskManager is already intialized, returning default mask %x", type->GetName(), s_DefaultMask);
			return s_DefaultMask;
		}
	}

	const DrawArgExtensionDesc& EntityMaskManager::GetExtensionDescFromExtensionFlag(uint32 flag)
	{
		ASSERT(s_ExtensionMaskToExtensionDescMap.contains(flag));
		return s_ExtensionMaskToExtensionDescMap[flag];
	}

	void EntityMaskManager::BindTypeToExtensionDesc(const ComponentType* type, DrawArgExtensionDesc extensionDesc, bool invertOnNewComponentType)
	{
		uint32 extensionFlag = GetExtensionFlag(type, invertOnNewComponentType);

		// Set extension description for later use
		auto eIt = s_ExtensionMaskToExtensionDescMap.find(extensionFlag);
		if (eIt == s_ExtensionMaskToExtensionDescMap.end())
			s_ExtensionMaskToExtensionDescMap[extensionFlag] = extensionDesc;
	}

	void EntityMaskManager::CopyDrawArgExtensionData(DrawArgExtensionData& dest, const DrawArgExtensionData* pSrc)
	{
		memcpy(&dest, pSrc, sizeof(DrawArgExtensionData));
	}
}
