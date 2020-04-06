#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class IBuffer;
	class ITexture;
	class IRenderPass;
	class IFrameBuffer;
	class ITextureView;
	class IPipelineState;
	class IDescriptorSet;
	class IPipelineLayout;
	class ICommandAllocator;
	class IBottomLevelAccelerationStructure;

	enum FCommandListFlags : uint32
	{
		COMMAND_LIST_FLAG_NONE				= 0,
		COMMAND_LIST_FLAG_ONE_TIME_SUBMIT	= FLAG(0),
	};

	enum FRenderPassBeginFlags : uint32
	{
		RENDER_PASS_BEGIN_FLAG_NONE					= 0,
		RENDER_PASS_BEGIN_FLAG_INLINE				= FLAG(0),
		RENDER_PASS_BEGIN_FLAG_EXECUTE_SECONDARY	= FLAG(1),
	};

	struct SecondaryCommandListBeginDesc
	{
		const IRenderPass*	pRenderPass		= nullptr;
		uint32				SubPass			= 0;
		const IFrameBuffer* pFrameBuffer	= nullptr;
	};

	struct CopyTextureFromBufferDesc
	{
		uint64 SrcOffset		= 0;
		uint64 SrcRowPitch		= 0;
		uint64 SrcHeight		= 0;
		uint32 Width			= 0; 
		uint32 Height			= 0; 
		uint32 Depth			= 0; 
		uint32 Miplevel			= 0; 
		uint32 MiplevelCount	= 0;
		uint32 ArrayIndex		= 0;
		uint32 ArrayCount		= 0;
	};
	
	struct PipelineTextureBarrier
	{
		ITexture*			pTexture		= nullptr;
		ETextureState		StateBefore		= ETextureState::TEXTURE_STATE_UNKNOWN;
		ETextureState		StateAfter		= ETextureState::TEXTURE_STATE_UNKNOWN;
		ECommandQueueType	QueueBefore		= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		ECommandQueueType	QueueAfter		= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		uint32				TextureFlags	= FTextureFlags::TEXTURE_FLAG_NONE;
		uint32				Miplevel		= 0;
		uint32				MiplevelCount	= 0;
		uint32				ArrayIndex		= 0;
		uint32				ArrayCount		= 0;
	};

	struct CommandListDesc
	{
        const char*         pName           = "";
		ECommandListType	CommandListType = ECommandListType::COMMANDLIST_UNKNOWN;
		uint32				Flags			= FCommandListFlags::COMMAND_LIST_FLAG_NONE;
	};

	class ICommandList : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandList);

		virtual void Begin(const SecondaryCommandListBeginDesc* pBeginDesc) = 0;
		
		virtual void Reset() = 0;
		virtual void End()	 = 0;

		virtual void BeginRenderPass(const IRenderPass* pRenderPass, const IFrameBuffer* pFrameBuffer, uint32 width, uint32 height, uint32 flags) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BuildTopLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)		= 0;
		virtual void BuildBottomLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)	= 0;

		virtual void CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes) = 0;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc) = 0;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount) = 0;

		virtual void GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter) = 0;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)           = 0;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)    = 0;
		
		virtual void SetConstantGraphics()	= 0;
		virtual void SetConstantCompute()	= 0;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer, uint64 offset) = 0;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) = 0;

		virtual void BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) = 0;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)	    = 0;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		= 0;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	= 0;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset)						= 0;
		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)	= 0;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)							= 0;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)	= 0;

		virtual void ExecuteSecondary(const ICommandList* pSecondary) = 0;

		virtual CommandListDesc	GetDesc()	const = 0;
		virtual uint64			GetHandle()	const = 0;

		//Should increase refcount -> Means that caller is responsible for calling Release
		virtual ICommandAllocator*	GetAllocator()	const = 0;
	};
}
