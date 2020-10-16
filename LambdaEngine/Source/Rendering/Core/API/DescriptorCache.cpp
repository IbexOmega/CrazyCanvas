#include "Rendering/Core/API/DescriptorCache.h"

#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{

	TSharedRef<DescriptorSet> DescriptorCache::GetDescriptorSet(const String& debugname, const PipelineLayout* pPipelineLayout, uint32 descriptorLayoutIndex, DescriptorHeap* pDescriptorHeap)
	{
		TSharedRef<DescriptorSet> ds;

		if (m_AvailableDescriptorSets.find(descriptorLayoutIndex) != m_AvailableDescriptorSets.end() && !m_AvailableDescriptorSets[descriptorLayoutIndex].IsEmpty())
		{
			ds = m_AvailableDescriptorSets[descriptorLayoutIndex].GetBack();
			m_AvailableDescriptorSets[descriptorLayoutIndex].PopBack();
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

		// Copy Descriptor set in use into Descriptor if it exists
		uint32 inUseIndex = 0;
		if (!m_NewDescriptorSets[descriptorLayoutIndex].IsEmpty())
		{
			inUseIndex = m_NewDescriptorSets[descriptorLayoutIndex].GetSize() - 1;
			if (inUseIndex < m_NewDescriptorSets[descriptorLayoutIndex].GetSize())
			{
				RenderAPI::GetDevice()->CopyDescriptorSet(m_NewDescriptorSets[descriptorLayoutIndex][inUseIndex].Get(), ds.Get());
			}
		}

		if (!m_InUseDescriptorSets[descriptorLayoutIndex].IsEmpty())
		{
			inUseIndex = m_InUseDescriptorSets[descriptorLayoutIndex].GetSize() - 1;
			if (inUseIndex < m_InUseDescriptorSets[descriptorLayoutIndex].GetSize())
			{
				RenderAPI::GetDevice()->CopyDescriptorSet(m_InUseDescriptorSets[descriptorLayoutIndex][inUseIndex].Get(), ds.Get());
			}
		}


		// Track Descriptor sets in use
		m_NewDescriptorSets[descriptorLayoutIndex].PushBack(ds);
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
			for (auto descriptorSet = setIndexArray.second.begin(); descriptorSet != setIndexArray.second.end();)
			{
				// Move to available list if 3 frames have pasted since Descriptor Set stopped being used
				if (descriptorSet->second == modFrameIndex)
				{
					m_AvailableDescriptorSets[setIndexArray.first].PushBack(descriptorSet->first);
					descriptorSet = setIndexArray.second.Erase(descriptorSet);
				}
				else
					descriptorSet++;
			}
		}
	}

	void DescriptorCache::HandleDirtyDescriptors()
	{
		for (auto dirtySetIndex : m_DirtySetIndices)
		{
			// Check if there is a descriptor sets in use on layout index and put it in unavailable list
			if (m_InUseDescriptorSets.find(dirtySetIndex) != m_InUseDescriptorSets.end() && !m_InUseDescriptorSets[dirtySetIndex].IsEmpty())
			{
				for (auto descriptorSet : m_InUseDescriptorSets[dirtySetIndex])
				{
					m_UnavailableDescriptorSets[dirtySetIndex].PushBack(std::make_pair(descriptorSet, m_CurrModFrameIndex));
				}
				m_InUseDescriptorSets[dirtySetIndex].Clear();
			}

			// Move new descriptor sets on layout index to inUse
			if (m_NewDescriptorSets.find(dirtySetIndex) != m_NewDescriptorSets.end() && !m_NewDescriptorSets[dirtySetIndex].IsEmpty())
			{
				for (auto descriptorSet : m_NewDescriptorSets[dirtySetIndex])
				{
					m_InUseDescriptorSets[dirtySetIndex].PushBack(descriptorSet);
				}
				m_NewDescriptorSets[dirtySetIndex].Clear();
			}
		}

		m_DirtySetIndices.clear();
	}

}