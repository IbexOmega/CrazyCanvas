#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

#define MAX_IMAGE_BARRIERS  16
#define MAX_BUFFER_BARRIERS	16
#define MAX_VIEWPORTS       8
#define MAX_VERTEX_BUFFERS  32

namespace LambdaEngine
{
	class IBuffer;
	class ITexture;
	class IQueryHeap;
	class IRenderPass;
	class ITextureView;
	class IPipelineState;
	class IDescriptorSet;
	class IPipelineLayout;
	class ICommandAllocator;
	class IAccelerationStructure;

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
		const IRenderPass*	        pRenderPass		    = nullptr;
		uint32				        SubPass			    = 0;
		const ITextureView* const * ppRenderTargets	    = nullptr;
        uint32                      RenderTargetCount   = 0;
        const ITextureView*         pDepthStencilView   = nullptr;
        uint32                      Width               = 0;
        uint32                      Height              = 0;
	};

#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(push)
	#pragma warning(disable : 4201) //Disable Warning - nonstandard extension used: nameless struct/union
#endif
	struct ClearColorDesc
	{
		union 
		{
			float Color[4];
			struct
			{
				float	Depth;
				uint32	Stencil;
			};
		};
	};
#ifdef LAMBDA_VISUAL_STUDIO
	#pragma warning(pop)
#endif

	struct BeginRenderPassDesc
	{
		const IRenderPass*			pRenderPass			= nullptr;
		const ITextureView* const *	ppRenderTargets		= nullptr;
		uint32						RenderTargetCount	= 0;
		const ITextureView*			pDepthStencil		= nullptr;
		uint32						Width				= 0;
		uint32						Height				= 0;
		uint32						Flags				= 0;
		const ClearColorDesc*		pClearColors		= nullptr;
		uint32						ClearColorCount		= 0;
		struct
		{
			int32 x = 0;
			int32 y = 0;
		} Offset;
	};

	struct CopyTextureFromBufferDesc
	{
		uint64 SrcOffset		= 0;
		uint64 SrcRowPitch		= 0;
		uint32 SrcHeight		= 0;
		uint32 Width			= 0; 
		uint32 Height			= 0; 
		uint32 Depth			= 0; 
		uint32 Miplevel			= 0; 
		uint32 MiplevelCount	= 0;
		uint32 ArrayIndex		= 0;
		uint32 ArrayCount		= 0;
	};
	
	struct PipelineTextureBarrierDesc
	{
		ITexture*			pTexture				= nullptr;
		ETextureState		StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		ETextureState		StateAfter				= ETextureState::TEXTURE_STATE_UNKNOWN;
		ECommandQueueType	QueueBefore				= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		ECommandQueueType	QueueAfter				= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		uint32				SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint32				DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint32				TextureFlags			= FTextureFlags::TEXTURE_FLAG_NONE;
		uint32				Miplevel				= 0;
		uint32				MiplevelCount			= 0;
		uint32				ArrayIndex				= 0;
		uint32				ArrayCount				= 0;
	};

	struct PipelineBufferBarrierDesc
	{
		IBuffer*			pBuffer					= nullptr;
		ECommandQueueType	QueueBefore				= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		ECommandQueueType	QueueAfter				= ECommandQueueType::COMMAND_QUEUE_UNKNOWN;
		uint32				SrcMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint32				DstMemoryAccessFlags	= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint64				Offset					= 0;
		uint64				SizeInBytes				= 0;
	};

	struct BuildTopLevelAccelerationStructureDesc
	{
		IAccelerationStructure* pAccelerationStructure	= nullptr;
		uint32					Flags					= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
		const IBuffer*			pInstanceBuffer			= nullptr;
		uint32					InstanceCount			= 0;
		bool					Update					= false;
	};

	struct BuildBottomLevelAccelerationStructureDesc
	{
		IAccelerationStructure*	pAccelerationStructure	= nullptr;
		uint32					Flags					= FAccelerationStructureFlags::ACCELERATION_STRUCTURE_FLAG_NONE;
		const IBuffer*			pVertexBuffer			= nullptr; 
		uint32					FirstVertexIndex		= 0; 
		uint32					VertexStride			= 0;
		const IBuffer*			pIndexBuffer			= nullptr;
		uint32					IndexBufferByteOffset	= 0; 
		uint32					TriangleCount			= 0;
		const void*				pTransform				= nullptr;
		bool					Update					= false;
	};

	struct CommandListDesc
	{
        const char*         pName           = "";
		ECommandListType	CommandListType = ECommandListType::COMMAND_LIST_UNKNOWN;
		uint32				Flags			= FCommandListFlags::COMMAND_LIST_FLAG_NONE;
	};

	class ICommandList : public IDeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(ICommandList);

		virtual bool Begin(const SecondaryCommandListBeginDesc* pBeginDesc) = 0;
		virtual bool End()	                                                = 0;

		virtual void BeginRenderPass(const BeginRenderPassDesc* pBeginDesc) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)		= 0;
		virtual void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc)	= 0;

		virtual void CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes)				= 0;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc)					= 0;
		virtual void BlitTexture(const ITexture* pSrc, ETextureState srcState, ITexture* pDst, ETextureState dstState, EFilter filter)	= 0;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrierDesc* pTextureBarriers, uint32 textureBarrierCount)	= 0;
		virtual void PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrierDesc* pBufferBarriers, uint32 bufferBarrierCount)		= 0;

		virtual void GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter) = 0;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)           = 0;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)    = 0;
		
		virtual void SetConstantRange(const IPipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset) = 0;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer, uint64 offset) = 0;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) = 0;

		virtual void BindDescriptorSetGraphics(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout, uint32 setIndex)	= 0;
		virtual void BindDescriptorSetCompute(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout, uint32 setIndex)		= 0;
		virtual void BindDescriptorSetRayTracing(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout, uint32 setIndex)	= 0;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)	    = 0;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		= 0;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	= 0;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset)						= 0;
		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)	= 0;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)							= 0;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)	= 0;
		virtual void DrawIndexedIndirect(const IBuffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)							= 0;

		virtual void BeginQuery(IQueryHeap* pQueryHeap, uint32 queryIndex)											= 0;
		virtual void Timestamp(IQueryHeap* pQueryHeap, uint32 queryIndex, FPipelineStageFlags pipelineStageFlag)	= 0;
		virtual void EndQuery(IQueryHeap* pQueryHeap, uint32 queryIndex)											= 0;

		virtual void ExecuteSecondary(const ICommandList* pSecondary) = 0;

		virtual CommandListDesc		GetDesc()	const = 0;
		virtual uint64				GetHandle()	const = 0;
		virtual ECommandQueueType	GetType()	const = 0;

		/*
		* Returns a pointer to the allocator used to allocate this commandlist. Caller should call Release on 
		* the returned pointer
		*
		* return - Returns a valid pointer if successful otherwise nullptr
		*/
		virtual ICommandAllocator*	GetAllocator()	const = 0;
	};
}
