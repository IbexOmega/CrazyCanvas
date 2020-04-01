#pragma once

#include "IDeviceChild.h"

namespace LambdaEngine
{
	class IBuffer;

	struct BottomLevelAccelerationStructureDesc
	{
		const char* pName		= "";
		uint32 MaxTriCount		= 0;
		uint32 MaxVertCount		= 0;
		bool Updateable			= false;
	};

	class IBottomLevelAccelerationStructure : public IDeviceChild
	{
	public:
		DECL_INTERFACE(IBottomLevelAccelerationStructure);

		virtual void UpdateGeometryData(IBuffer* pVertexBuffer, uint32 firstVertexIndex, uint32 vertexSize, IBuffer* pIndexBuffer, uint32 indexBufferByteOffset, uint32 triCount, void* pTransform, IBuffer* pScratchBuffer) = 0;

		virtual uint64 GetScratchMemorySizeRequirement() = 0;
	};
}
