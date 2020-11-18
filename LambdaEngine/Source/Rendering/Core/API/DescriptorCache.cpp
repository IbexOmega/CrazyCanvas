#include "Rendering/Core/API/DescriptorCache.h"

#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{

	TSharedRef<DescriptorSet> DescriptorCache::GetDescriptorSet(const String& debugname, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap, bool shouldCopy)
	{
		TSharedRef<DescriptorSet> ds;

		if (auto availableDescriptorSetsIt = m_AvailableDescriptorSets.find(descriptorLayoutIndex);
			availableDescriptorSetsIt != m_AvailableDescriptorSets.end() && !availableDescriptorSetsIt->second.IsEmpty())
		{
			ds = availableDescriptorSetsIt->second.GetBack();
			availableDescriptorSetsIt->second.PopBack();
		}
		else
		{
			ds = RenderAPI::GetDevice()->CreateDescriptorSet(debugname, pPipelineLayout, descriptorLayoutIndex, pDescriptorHeap);

			if (ds == nullptr)
			{
				LOG_ERROR("[DescriptorCache]: Failed to create descriptor set[%d]", descriptorLayoutIndex);
				return nullptr;
			}
		}

		TArray<TSharedRef<DescriptorSet>>& newDescriptorSets = m_NewDescriptorSets[descriptorLayoutIndex];

		if (shouldCopy)
		{
			// Copy Descriptor set in use into Descriptor if it exists
			uint32 inUseIndex = 0;

			TArray<TSharedRef<DescriptorSet>>& inUseDescriptorSets = m_InUseDescriptorSets[descriptorLayoutIndex];
	
			if (!inUseDescriptorSets.IsEmpty())
			{
				inUseIndex = inUseDescriptorSets.GetSize() - 1;
				if (inUseIndex < inUseDescriptorSets.GetSize())
				{
					RenderAPI::GetDevice()->CopyDescriptorSet(inUseDescriptorSets[inUseIndex].Get(), ds.Get());
				}
			}
			
			if (!newDescriptorSets.IsEmpty())
			{
				inUseIndex = newDescriptorSets.GetSize() - 1;
				if (inUseIndex < newDescriptorSets.GetSize())
				{
					RenderAPI::GetDevice()->CopyDescriptorSet(newDescriptorSets[inUseIndex].Get(), ds.Get());
				}
			}
		}

		// Track Descriptor sets in use
		newDescriptorSets.PushBack(ds);
		m_DirtySetIndices.insert(descriptorLayoutIndex);

		return ds;
	}

	void LambdaEngine::DescriptorCache::HandleUnavailableDescriptors(uint32 modFrameIndex)
	{
		// Move descriptor sets in use to unavailable 
		HandleDirtyDescriptors();

		m_CurrModFrameIndex = modFrameIndex;

		// Go through descriptorSet and see if they are still in use
		for (auto& setIndexArray : m_UnavailableDescriptorSets)
		{
			for (auto descriptorSetIt = setIndexArray.second.begin(); descriptorSetIt != setIndexArray.second.end();)
			{
				// Move descriptorSetIt available list if 3 frames have pasted since Descriptor Set stopped being used
				if (descriptorSetIt->second == modFrameIndex)
				{
					m_AvailableDescriptorSets[setIndexArray.first].PushBack(descriptorSetIt->first);
					descriptorSetIt = setIndexArray.second.Erase(descriptorSetIt);
				}
				else
					descriptorSetIt++;
			}
		}
	}

	void DescriptorCache::LogStatistics(const String& cacheName)
	{
		uint32 newCount = 0;
		uint32 inUseCount = 0;
		uint32 unavailableCount = 0;
		uint32 availableCount = 0;

		for (auto newDescriptorSetPair : m_NewDescriptorSets)					newCount			+= newDescriptorSetPair.second.GetSize();
		for (auto inUseDescriptorSetPair : m_InUseDescriptorSets)				inUseCount			+= inUseDescriptorSetPair.second.GetSize();
		for (auto unavailableDescriptorSetPair : m_UnavailableDescriptorSets)	unavailableCount	+= unavailableDescriptorSetPair.second.GetSize();
		for (auto availableDescriptorSetPair : m_AvailableDescriptorSets)		availableCount		+= availableDescriptorSetPair.second.GetSize();

		LOG_INFO("[DescriptorCache]: %s", cacheName.c_str());
		LOG_INFO("\t #New DS: %u", newCount);
		LOG_INFO("\t #In Use DS: %u", inUseCount);
		LOG_INFO("\t #Unavailable DS: %u", unavailableCount);
		LOG_INFO("\t #Available DS: %u", availableCount);
		LOG_INFO("\t #Total DS: %u\n", newCount + inUseCount + unavailableCount + availableCount);
	}

	void DescriptorCache::HandleDirtyDescriptors()
	{
		for (auto dirtySetIndex : m_DirtySetIndices)
		{
			// Check if there is a descriptor sets in use on layout index and put it in unavailable list
			if (auto inUseDescriptorSetIt = m_InUseDescriptorSets.find(dirtySetIndex); 
				inUseDescriptorSetIt != m_InUseDescriptorSets.end() && !inUseDescriptorSetIt->second.IsEmpty())
			{
				TArray<std::pair<TSharedRef<DescriptorSet>, ReleaseFrame>>& unavailableDescriptorSets = m_UnavailableDescriptorSets[dirtySetIndex];

				for (auto descriptorSet : inUseDescriptorSetIt->second)
				{
					unavailableDescriptorSets.PushBack(std::make_pair(descriptorSet, m_CurrModFrameIndex));
				}

				inUseDescriptorSetIt->second.Clear();
			}

			// Move new descriptor sets on layout index to inUse
			if (auto newDescriptorSetIt = m_NewDescriptorSets.find(dirtySetIndex);
				newDescriptorSetIt != m_NewDescriptorSets.end() && !newDescriptorSetIt->second.IsEmpty())
			{
				TArray<TSharedRef<DescriptorSet>>& inUseDescriptorSets = m_InUseDescriptorSets[dirtySetIndex];

				for (auto descriptorSet : newDescriptorSetIt->second)
				{
					inUseDescriptorSets.PushBack(descriptorSet);
				}

				newDescriptorSetIt->second.Clear();
			}
		}

		m_DirtySetIndices.clear();
	}

}