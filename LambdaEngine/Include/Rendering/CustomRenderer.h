#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Rendering/Core/API/GraphicsTypes.h"
#include "Containers/String.h"

namespace LambdaEngine
{
	struct RenderPassAttachmentDesc;

	class RenderGraph;
	class CommandAllocator;
	class CommandList;
	class TextureView;
	class Sampler;
	class Buffer;
	class AccelerationStructure;
	struct DrawArg;

	struct CustomRendererRenderGraphInitDesc
	{
		RenderGraph*				pRenderGraph					= nullptr;
		uint32						BackBufferCount					= 0;
		RenderPassAttachmentDesc*	pColorAttachmentDesc			= nullptr;
		uint32						ColorAttachmentCount			= 0;
		RenderPassAttachmentDesc*	pDepthStencilAttachmentDesc		= nullptr;
	};

	class CustomRenderer
	{
	public:
		DECL_INTERFACE(CustomRenderer);

		/*
		* Called once by RenderSystem, required arguments should be passed in the constructor.
		*/
		virtual bool Init() = 0;

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
		{
			UNREFERENCED_VARIABLE(pPreInitDesc);
			return true;
		}

		virtual bool RenderGraphPostInit() { return true; }

		/*
		* Called every frame, can be used for internal resource handling in custom renderers
		*/
		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
		{
			UNREFERENCED_VARIABLE(delta);
			UNREFERENCED_VARIABLE(modFrameIndex);
			UNREFERENCED_VARIABLE(backBufferIndex);
		}

		/*
		* Called before when a/multiple buffer descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreBuffersDescriptorSetWrite() {}
		/*
		* Called before when a/multiple texture descriptor write(s) are discovered.
		* Allows the callee to recreate Descriptor Sets if needed.
		*/
		virtual void PreTexturesDescriptorSetWrite() {}

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given texture
		*	resourceName - Name of the resource being updated
		*	ppPerImageTextureViews - An array of textureviews that represent the update data, this is per Texture (or Image), normally, use this when writing to Descriptor Set
		*	ppPerSubImageTextureViews - An array of textureviews that represent the update data, this is per Sub Texture (or Sub Image), normally, use these when binding to FBO
		*	imageCount - Size of ppPerImageTextureViews
		*	subImageCount - Size of ppPerSubImageTextureViews, guaranteed to be an integer multiple of imageCount
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateTextureResource(
			const String& resourceName,
			const TextureView* const* ppPerImageTextureViews,
			const TextureView* const* ppPerSubImageTextureViews,
			const Sampler* const* ppPerImageSamplers,
			uint32 imageCount,
			uint32 subImageCount,
			bool backBufferBound)
		{
			UNREFERENCED_VARIABLE(resourceName);
			UNREFERENCED_VARIABLE(ppPerImageTextureViews);
			UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
			UNREFERENCED_VARIABLE(ppPerImageSamplers);
			UNREFERENCED_VARIABLE(imageCount);
			UNREFERENCED_VARIABLE(subImageCount);
			UNREFERENCED_VARIABLE(backBufferBound);
		}

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given buffer
		*	resourceName - Name of the resource being updated
		*	ppBuffers - An array of buffers that represent the update data
		*	pOffsets - An array of offsets into each ppBuffers entry
		*	pSizesInBytes - An array of sizes representing the data size of each target sub entry in each ppBuffers entry
		*	count - size of ppBuffers, pOffsets and pSizesInBytes
		*	backBufferBound - Describes if subresources are bound in array or 1 / Back buffer
		*/
		virtual void UpdateBufferResource(
			const String& resourceName,
			const Buffer* const* ppBuffers,
			uint64* pOffsets,
			uint64* pSizesInBytes,
			uint32 count,
			bool backBufferBound)
		{
			UNREFERENCED_VARIABLE(resourceName);
			UNREFERENCED_VARIABLE(ppBuffers);
			UNREFERENCED_VARIABLE(pOffsets);
			UNREFERENCED_VARIABLE(pSizesInBytes);
			UNREFERENCED_VARIABLE(count);
			UNREFERENCED_VARIABLE(backBufferBound);
		}

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given acceleration structure
		*	resourceName - Name of the resource being updated
		*	pAccelerationStructure - The acceleration structure that represent the update data
		*/
		virtual void UpdateAccelerationStructureResource(
			const String& resourceName,
			const AccelerationStructure* const* pAccelerationStructure)
		{
			UNREFERENCED_VARIABLE(resourceName);
			UNREFERENCED_VARIABLE(pAccelerationStructure);
		}

		/*
		* Called when a ResourceUpdate is scheduled in the RenderGraph for the given draw arg
		*	resourceName - Name of the resource being updated
		*	drawArgs - The draw args that represent the update data
		*/
		virtual void UpdateDrawArgsResource(
			const String& resourceName,
			const DrawArg* pDrawArgs,
			uint32 count)
		{
			UNREFERENCED_VARIABLE(resourceName);
			UNREFERENCED_VARIABLE(pDrawArgs);
			UNREFERENCED_VARIABLE(count);
		}

		/*
		* Called at rendertime to allow recording device commands
		*	modFrameIndex - The current Frame Index % #BackBufferImages
		*	backBufferIndex - The current Back Buffer index
		*	pFirstExecutionStage - A pointer to a variable which should contain the recorded command buffer which will be executed first, if this rendering should be executed.
		*	pSecondaryExecutionStage - A pointer to a variable which should contain the recorded command buffer which will be executed after pFirstExecutionStage, if this rendering should be executed.
		*		Example:
		*		if (executeRecording)	(*ppExecutionStage) = pCommandList;
		*		else					(*ppExecutionStage) = nullptr;
		*/
		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping)		= 0;

		virtual FPipelineStageFlag GetFirstPipelineStage() const	= 0;
		virtual FPipelineStageFlag GetLastPipelineStage() const		= 0;

		virtual const String& GetName() const = 0;
	};
}

