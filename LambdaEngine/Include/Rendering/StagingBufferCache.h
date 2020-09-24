#pragma once

#include "Rendering/Core/API/GraphicsTypes.h"

#include "Time/API/Timestamp.h"

#include "Containers/TArray.h"

#include <map>

namespace LambdaEngine
{
	class Buffer;

	class StagingBufferCache
	{
		struct SizeCompare
		{
			bool operator()(uint64 requiredSize, uint64 bufferSize) const
			{
				return requiredSize <= bufferSize;
			}
		};

	public:
		DECL_STATIC_CLASS(StagingBufferCache);

		static bool Release();
		static void PreRender();

		/*
		*	Returns a Staging Buffer which should be used this frame.
		*	The size of the buffer is guaranteed to be larger than or equal to the requested size.
		*/
		static Buffer* RequestBuffer(uint64 size);

	private:
		static std::multimap<uint64, Buffer*, SizeCompare> s_AvailableBuffers[BACK_BUFFER_COUNT];
		static TArray<Buffer*>	s_StagedBuffers[BACK_BUFFER_COUNT];
		static uint32 s_ModFrameIndex;
	};
}