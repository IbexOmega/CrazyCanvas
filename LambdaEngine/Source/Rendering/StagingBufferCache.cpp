#include "Rendering/StagingBufferCache.h"

#include "Rendering/RenderAPI.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/Buffer.h"

namespace LambdaEngine
{
	std::multimap<uint64, Buffer*, StagingBufferCache::SizeCompare> StagingBufferCache::s_AvailableBuffers[BACK_BUFFER_COUNT];
	TArray<Buffer*> StagingBufferCache::s_StagedBuffers[BACK_BUFFER_COUNT];
	uint32 StagingBufferCache::s_ModFrameIndex = 0;

	bool StagingBufferCache::Release()
	{
		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<Buffer*>& frameStagedBuffers = s_StagedBuffers[b];

			for (Buffer* pBuffer : frameStagedBuffers)
			{
				SAFERELEASE(pBuffer);
			}

			frameStagedBuffers.Clear();

			std::multimap<uint64, Buffer*, SizeCompare>& frameAvailableBuffers = s_AvailableBuffers[b];

			for (auto bufferIt = frameAvailableBuffers.begin(); bufferIt != frameAvailableBuffers.end(); bufferIt++)
			{
				SAFERELEASE(bufferIt->second);
			}

			frameAvailableBuffers.clear();
		}

		return true;
	}

	void StagingBufferCache::PreRender()
	{
		TArray<Buffer*>& frameStagedBuffers = s_StagedBuffers[s_ModFrameIndex];

		//Reinsert all previously used staging buffers into available buffers
		if (!frameStagedBuffers.IsEmpty())
		{
			std::multimap<uint64, Buffer*, SizeCompare>& frameAvailableBuffers = s_AvailableBuffers[s_ModFrameIndex];

			for (Buffer* pFinishedStagingBuffer : frameStagedBuffers)
			{
				frameAvailableBuffers.insert({ pFinishedStagingBuffer->GetDesc().SizeInBytes, pFinishedStagingBuffer });
			}

			frameStagedBuffers.Clear();
		}

		s_ModFrameIndex++;
		if (s_ModFrameIndex >= BACK_BUFFER_COUNT) s_ModFrameIndex = 0;
	}

	Buffer* StagingBufferCache::RequestBuffer(uint64 size)
	{
		Buffer* pReturnBuffer;

		std::multimap<uint64, Buffer*, SizeCompare>& frameAvailableBuffers = s_AvailableBuffers[s_ModFrameIndex];

		auto availableBufferIt = frameAvailableBuffers.find(size);
		if (availableBufferIt == frameAvailableBuffers.end())
		{
			//If we don't find one, create a new 
			BufferDesc bufferDesc = {};
			bufferDesc.DebugName	= "Staging Buffer Cache";
			bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			bufferDesc.SizeInBytes	= size;

			pReturnBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			s_StagedBuffers[s_ModFrameIndex].PushBack(pReturnBuffer);
		}
		else
		{
			//If we find one, move it to m_StagedBuffers
			pReturnBuffer = availableBufferIt->second;
			s_StagedBuffers[s_ModFrameIndex].PushBack(pReturnBuffer);
			s_AvailableBuffers->erase(availableBufferIt);
		}

		return pReturnBuffer;
	}
}