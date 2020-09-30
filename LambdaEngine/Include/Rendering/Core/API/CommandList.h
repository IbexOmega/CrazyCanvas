#pragma once
#include "DeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class SBT;
	class Buffer;
	class Sampler;
	class Texture;
	class QueryHeap;
	class RenderPass;
	class TextureView;
	class PipelineState;
	class DescriptorSet;
	class PipelineLayout;
	class CommandAllocator;
	class AccelerationStructure;

	typedef uint32 FCommandListFlags;
	enum FCommandListFlag : FCommandListFlags
	{
		COMMAND_LIST_FLAG_NONE				= 0,
		COMMAND_LIST_FLAG_ONE_TIME_SUBMIT	= FLAG(0),
	};

	typedef uint32 FRenderPassBeginFlags;
	enum FRenderPassBeginFlag : FRenderPassBeginFlags
	{
		RENDER_PASS_BEGIN_FLAG_NONE					= 0,
		RENDER_PASS_BEGIN_FLAG_INLINE				= FLAG(0),
		RENDER_PASS_BEGIN_FLAG_EXECUTE_SECONDARY	= FLAG(1),
	};

	struct SecondaryCommandListBeginDesc
	{
		const RenderPass*			pRenderPass			= nullptr;
		uint32						SubPass				= 0;
		const TextureView* const *	ppRenderTargets		= nullptr;
		uint32						RenderTargetCount	= 0;
		const TextureView*			pDepthStencilView	= nullptr;
		uint32						Width				= 0;
		uint32						Height				= 0;
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
		const RenderPass*			pRenderPass			= nullptr;
		const TextureView* const *	ppRenderTargets		= nullptr;
		uint32						RenderTargetCount	= 0;
		const TextureView*			pDepthStencil		= nullptr;
		uint32						Width				= 0;
		uint32						Height				= 0;
		FRenderPassBeginFlags		Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_NONE;
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
		uint32 OffsetX			= 0;
		uint32 OffsetY			= 0;
		uint32 OffsetZ			= 0;
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
		Texture*			pTexture				= nullptr;
		ETextureState		StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		ETextureState		StateAfter				= ETextureState::TEXTURE_STATE_UNKNOWN;
		ECommandQueueType	QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		ECommandQueueType	QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		FMemoryAccessFlags	SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		FMemoryAccessFlags	DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		FTextureFlags		TextureFlags			= FTextureFlag::TEXTURE_FLAG_NONE;
		uint32				Miplevel				= 0;
		uint32				MiplevelCount			= 0;
		uint32				ArrayIndex				= 0;
		uint32				ArrayCount				= 0;
	};

	struct PipelineBufferBarrierDesc
	{
		Buffer*				pBuffer					= nullptr;
		ECommandQueueType	QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		ECommandQueueType	QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		FMemoryAccessFlags	SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		FMemoryAccessFlags	DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		uint64				Offset					= 0;
		uint64				SizeInBytes				= 0;
	};

	struct PipelineMemoryBarrierDesc
	{
		FMemoryAccessFlags SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
		FMemoryAccessFlags DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_UNKNOWN;
	};

	struct BuildTopLevelAccelerationStructureDesc
	{
		AccelerationStructure*		pAccelerationStructure	= nullptr;
		FAccelerationStructureFlags	Flags					= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		const Buffer*				pInstanceBuffer			= nullptr;
		uint32						InstanceCount			= 0;
		bool						Update					= false;
	};

	struct BuildBottomLevelAccelerationStructureDesc
	{
		AccelerationStructure*		pAccelerationStructure	= nullptr;
		FAccelerationStructureFlags	Flags					= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_NONE;
		const Buffer*				pVertexBuffer			= nullptr;
		uint32						FirstVertexIndex		= 0;
		uint32						VertexStride			= 0;
		const Buffer*				pIndexBuffer			= nullptr;
		uint32						IndexBufferByteOffset	= 0;
		uint32						TriangleCount			= 0;
		const Buffer*				pTransformBuffer		= nullptr;
		uint32						TransformByteOffset		= 0;
		bool						Update					= false;
	};

	struct CommandListDesc
	{
		String				DebugName		= "";
		ECommandListType	CommandListType = ECommandListType::COMMAND_LIST_TYPE_UNKNOWN;
		FCommandListFlags	Flags			= FCommandListFlag::COMMAND_LIST_FLAG_NONE;
	};


	class CommandList : public DeviceChild
	{
	public:
		DECL_DEVICE_INTERFACE(CommandList);

		virtual bool Begin(const SecondaryCommandListBeginDesc* pBeginDesc) = 0;
		virtual bool End()													= 0;

		virtual void BeginRenderPass(const BeginRenderPassDesc* pBeginDesc) = 0;
		virtual void EndRenderPass() = 0;

		virtual void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)		= 0;
		virtual void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc)	= 0;

		virtual void CopyBuffer(const Buffer* pSrc, uint64 srcOffset, Buffer* pDst, uint64 dstOffset, uint64 sizeInBytes)					= 0;
		virtual void CopyTextureFromBuffer(const Buffer* pSrc, Texture* pDst, const CopyTextureFromBufferDesc& desc)						= 0;
		virtual void BlitTexture(const Texture* pSrc, ETextureState srcState, Texture* pDst, ETextureState dstState, EFilterType filter)	= 0;

		virtual void TransitionBarrier(Texture* resource, FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, uint32 srcAccessMask, uint32 destAccessMask, ETextureState beforeState, ETextureState afterState) = 0;
		virtual void TransitionBarrier(Texture* resource, FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, uint32 srcAccessMask, uint32 destAccessMask, uint32 arrayIndex, uint32 arrayCount, ETextureState beforeState, ETextureState afterState) = 0;

		virtual void QueueTransferBarrier(Texture* resource, FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, uint32 srcAccessMask, uint32 destAccessMask, ECommandQueueType srcQueue, ECommandQueueType dstQueue, ETextureState beforeState, ETextureState afterState) = 0;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrierDesc* pTextureBarriers, uint32 textureBarrierCount)	= 0;
		virtual void PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrierDesc* pBufferBarriers, uint32 bufferBarrierCount)		= 0;
		virtual void PipelineMemoryBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineMemoryBarrierDesc* pMemoryBarriers, uint32 bufferMemoryCount)			= 0;

		virtual void GenerateMiplevels(Texture* pTexture, ETextureState stateBefore, ETextureState stateAfter) = 0;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)			= 0;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)	= 0;
		virtual void SetStencilTestEnabled(bool enabled) = 0;
		virtual void SetStencilTestOp(EStencilFace face, EStencilOp failOp, EStencilOp passOp, EStencilOp depthFailOp, ECompareOp compareOp) = 0;
		virtual void SetStencilTestReference(EStencilFace face, uint32 reference) = 0;
		
		virtual void SetConstantRange(const PipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset) = 0;

		virtual void BindIndexBuffer(const Buffer* pIndexBuffer, uint64 offset, EIndexType indexType) = 0;
		virtual void BindVertexBuffers(const Buffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) = 0;

		virtual void BindDescriptorSetGraphics(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)		= 0;
		virtual void BindDescriptorSetCompute(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)		= 0;
		virtual void BindDescriptorSetRayTracing(const DescriptorSet* pDescriptorSet, const PipelineLayout* pPipelineLayout, uint32 setIndex)	= 0;

		virtual void BindGraphicsPipeline(const PipelineState* pPipeline)	= 0;
		virtual void BindComputePipeline(const PipelineState* pPipeline)	= 0;
		virtual void BindRayTracingPipeline(PipelineState* pPipeline)		= 0;

		virtual void TraceRays(const SBT* pSBT, uint32 width, uint32 height, uint32 depth) = 0;

		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) = 0;

		virtual void DispatchMesh(uint32 taskCount, uint32 firstTask)													= 0;
		virtual void DispatchMeshIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)	= 0;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)							= 0;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)	= 0;
		virtual void DrawIndexedIndirect(const Buffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)								= 0;

		virtual void BeginQuery(QueryHeap* pQueryHeap, uint32 queryIndex)										= 0;
		virtual void Timestamp(QueryHeap* pQueryHeap, uint32 queryIndex, FPipelineStageFlags pipelineStageFlag)	= 0;
		virtual void EndQuery(QueryHeap* pQueryHeap, uint32 queryIndex)											= 0;
		virtual void ResetQuery(QueryHeap* pQueryHeap, uint32 firstQuery, uint32 queryCount) = 0;

		virtual void DeferDestruction(DeviceChild* pResource) = 0;

		virtual void ExecuteSecondary(const CommandList* pSecondary) = 0;

		virtual void FlushDeferredBarriers()	= 0;
		virtual void FlushDeferredResources()	= 0;

		virtual uint64 GetHandle() const = 0;

		FORCEINLINE const CommandListDesc& GetDesc() const
		{
			return m_Desc;
		}

		FORCEINLINE ECommandQueueType GetType()	const
		{
			return m_QueueType;
		}

		FORCEINLINE bool IsBegin() const { return m_IsBegin; }

		/*
		* Returns a pointer to the allocator used to allocate this commandlist. Caller should call Release on
		* the returned pointer
		*	return - Returns a valid pointer if successful otherwise nullptr
		*/
		virtual CommandAllocator* GetAllocator() = 0;

	protected:
		ECommandQueueType	m_QueueType = ECommandQueueType::COMMAND_QUEUE_TYPE_UNKNOWN;
		CommandListDesc		m_Desc;
		bool				m_IsBegin = false;
	};
}
