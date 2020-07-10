#pragma once

#include "LambdaEngine.h"
#include "Time/API/Timestamp.h"

#include "Rendering/Core/API/GraphicsTypes.h"

namespace LambdaEngine
{
	struct RenderPassAttachmentDesc;

	class ICommandAllocator;
	class ICommandList;
	class ITextureView;
	class IBuffer;
	class IAccelerationStructure;

	struct CustomRendererRenderGraphInitDesc
	{
		RenderPassAttachmentDesc*	pColorAttachmentDesc			= nullptr;
		uint32						ColorAttachmentCount			= 0;
		RenderPassAttachmentDesc*	pDepthStencilAttachmentDesc		= nullptr;
	};

	class ICustomRenderer
	{
	public:
		DECL_INTERFACE(ICustomRenderer);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) = 0;

		virtual void PreBuffersDescriptorSetWrite()		= 0;
		virtual void PreTexturesDescriptorSetWrite()	= 0;

		virtual void UpdateParameters(void* pData)		= 0;

		virtual void UpdatePushConstants(void* pData, uint32 dataSize)	= 0;

		virtual void UpdateTextureResource(const String& resourceName, const ITextureView* const * ppTextureViews, uint32 count)	= 0;
		virtual void UpdateBufferResource(const String& resourceName, const IBuffer* const * ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count)	= 0;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const IAccelerationStructure* pAccelerationStructure)	= 0;

		virtual void NewFrame(Timestamp delta)		= 0;
		virtual void PrepareRender(Timestamp delta)		= 0;

		virtual void Render(ICommandAllocator* pCommandAllocator, ICommandList* pCommandList, ICommandList** ppExecutionStage, uint32 modFrameIndex, uint32 backBufferIndex)		= 0;

		virtual FPipelineStageFlags GetFirstPipelineStage()	= 0;
		virtual FPipelineStageFlags GetLastPipelineStage()	= 0;
	};
}

