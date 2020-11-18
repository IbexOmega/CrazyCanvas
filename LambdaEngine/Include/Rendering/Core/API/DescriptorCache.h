#pragma once

#include "Rendering/RenderGraphTypes.h"

namespace LambdaEngine
{
	class PipelineLayout;
	class DescriptorHeap;

	/*
	* Cache used for repurposing descriptor sets instead of creating new when descriptors needs to be updated.
	* Use GetDescriptorSet to create/repurpose descriptor set
	* !! Remember to call HandleUnavailableDescriptors every frame !!
	*/
	class DescriptorCache
	{
	public:
		using ReleaseFrame = uint32;
		using DescriptorSetIndex = uint32;
		using DirtyDescriptorSets = uint32;

	public:
		DescriptorCache() = default;
		~DescriptorCache() = default;

		/*
		*	Used to either create or repurpose an already existing descriptor sets
		*	*Caution* All Descriptor sets created on this LayoutIndex will be prepared for repurposing
		*/
		TSharedRef<DescriptorSet> GetDescriptorSet(const String& debugname, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap, bool shouldCopy = true);

		/*
		*	Checks if unavailable descriptor sets can be set to available
		*/
		void HandleUnavailableDescriptors(uint32 modFrameIndex);

		void LogStatistics(const String& cacheName);

	private:
		void HandleDirtyDescriptors();

	private:
		uint32	m_CurrModFrameIndex = 0;

		/*
			Used when new descriptors are created and the old ones should be moved to unavailable
		*/
		std::set<DirtyDescriptorSets>																m_DirtySetIndices;
		THashTable<DescriptorSetIndex, TArray<TSharedRef<DescriptorSet>>>							m_NewDescriptorSets;
		THashTable<DescriptorSetIndex, TArray<TSharedRef<DescriptorSet>>>							m_InUseDescriptorSets;
		
		THashTable<DescriptorSetIndex, TArray<std::pair<TSharedRef<DescriptorSet>, ReleaseFrame>>>	m_UnavailableDescriptorSets;
		THashTable<DescriptorSetIndex, TArray<TSharedRef<DescriptorSet>>>							m_AvailableDescriptorSets;
	};

}