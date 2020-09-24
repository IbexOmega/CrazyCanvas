#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	struct RenderPassAttachmentDesc;

	class CommandAllocator;
	class CommandList;
	class TextureView;
	class Buffer;
	class AccelerationStructure;

	struct CustomRendererRenderGraphInitDesc
	{
		uint32						BackBufferCount					= 0;
		RenderPassAttachmentDesc*	pColorAttachmentDesc			= nullptr;
		uint32						ColorAttachmentCount			= 0;
		RenderPassAttachmentDesc*	pDepthStencilAttachmentDesc		= nullptr;
	};

	class ICustomRenderer
	{
	public:
		DECL_INTERFACE(ICustomRenderer);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) = 0;

		/*
		* Called before when a/multiple buffer descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreBuffersDescriptorSetWrite()		= 0;
		/*
		* Called before when a/multiple texture descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreTexturesDescriptorSetWrite()	= 0;

		/*
		* Called when the Render Graph discovers a parameter update for the custom renderer.
		*	pData - The data passed to RenderGraph::UpdateRenderStageParameters
		*/
		//virtual void UpdateParameters(void* pData)		= 0;

		//virtual void UpdatePushConstants(void* pData, uint32 dataSize)	= 0;

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given texture
		*	resourceName - Name of the resource being updated
		*	ppTextureViews - An array of textureviews that represent the update data
		*	count - Size of ppTextureViews
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const * ppTextureViews, uint32 count, bool backBufferBound)	= 0;
		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given buffer
		*	resourceName - Name of the resource being updated
		*	ppBuffers - An array of buffers that represent the update data
		*	pOffsets - An array of offsets into each ppBuffers entry
		*	pSizesInBytes - An array of sizes representing the data size of each target sub entry in each ppBuffers entry
		*	count - size of ppBuffers, pOffsets and pSizesInBytes
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const * ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)	= 0;
		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given acceleration structure
		*	resourceName - Name of the resource being updated
		*	pAccelerationStructure - The acceleration structure that represent the update data
		*/
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)	= 0;

		/*
		* Called at rendertime to allow recording device commands
		*	pGraphicsCommandAllocator - The graphics command allocator to be reset
		*	pGraphicsCommandList - The graphics command list corresponding to pCommandAllocator and which to record device commands to
		*	pComputeCommandAllocator - The compute command allocator to be reset
		*	pComputeCommandList - The compute command list corresponding to pCommandAllocator and which to record device commands to
		*	modFrameIndex - The current Frame Index % #BackBufferImages
		*	backBufferIndex - The current Back Buffer index
		*	ppFirstExecutionStage - A pointer to a variable which should contain the recorded command buffer which will be executed first, if this rendering should be executed.
		*	ppSecondaryExecutionStage - A pointer to a variable which should contain the recorded command buffer which will be executed secondary, if this rendering should be executed.
		*		Example:	
		*		if (executeRecording)	(*ppExecutionStage) = pCommandList;
		*		else					(*ppExecutionStage) = nullptr;
		*/
		virtual void Render(
			CommandAllocator* pGraphicsCommandAllocator, 
			CommandList* pGraphicsCommandList, 
			CommandAllocator* pComputeCommandAllocator, 
			CommandList* pComputeCommandList, 
			uint32 modFrameIndex, 
			uint32 backBufferIndex, 
			CommandList** ppPrimaryExecutionStage, 
			CommandList** ppSecondaryExecutionStage)		= 0;

		virtual FPipelineStageFlag GetFirstPipelineStage()	= 0;
		virtual FPipelineStageFlag GetLastPipelineStage()	= 0;

		virtual const String& GetName() const = 0;
	};
}

