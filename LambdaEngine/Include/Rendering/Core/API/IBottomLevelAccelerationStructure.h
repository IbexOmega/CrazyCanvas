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
		DECL_DEVICE_INTERFACE(IBottomLevelAccelerationStructure);

		/*
		* Updates the contained geometric data withing the Acceleration Structure, the vertex position data is assumed to be in 3 * F32 style, and the indices are assumed to be of type U32.
		* This can only be called once if the BottomLevelAS is initialized with "Updateable" = false
		*	pVertexBuffer - A graphics device buffer containing vertices
		*	firstVertexIndex - An index to the first vertex in pVertexBuffer belonging to this BottomLevelAS
		*	pIndexBuffer - A graphics device buffer containing indices
		*	indexBufferByteOffset - A byteoffset in the pIndexBuffer to the first index belonging to this BottomLevelAS
		*	triCount - The number of triangles belonging to this BottomLevelAS
		*	pTransform - An optional transform, this transform will be used 'offline'
		*	pScratchBuffer -  A graphics device buffer that must be atlest the size returned by GetScratchMemorySizeRequirement
		*/
		virtual void UpdateGeometryData(IBuffer* pVertexBuffer, uint32 firstVertexIndex, IBuffer* pIndexBuffer, uint32 indexBufferByteOffset, uint32 triCount, void* pTransform, IBuffer* pScratchBuffer) = 0;

		/*
		* Getter for the minimum memory size requirement of a scratch buffer that will be used to build/refit this BottomLevelAS
		*
		* return - The minimum size in bytes
		*/
		virtual uint64 GetScratchMemorySizeRequirement() = 0;
	};
}
