#pragma once
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/TDeviceChildBase.h"

#include "Vulkan.h"

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class CommandAllocatorVK;
    class RayTracingPipelineStateVK;

	class CommandListVK : public TDeviceChildBase<GraphicsDeviceVK, ICommandList>
	{
		using TDeviceChild = TDeviceChildBase<GraphicsDeviceVK, ICommandList>;

	public:
		CommandListVK(const GraphicsDeviceVK* pDevice);
		~CommandListVK();

		bool Init(ICommandAllocator* pAllocator, const CommandListDesc& desc);

		FORCEINLINE VkCommandBuffer GetCommandBuffer() const
		{
			return m_CommandList;
		}

		//IDeviceChild interface
		virtual void SetName(const char* pName) override;

		//ICommandList interface
		virtual void Begin(const SecondaryCommandListBeginDesc* pBeginDesc)	override final;

		virtual void Reset() override final;
		virtual void End()	 override final;

		virtual void BeginRenderPass(const BeginRenderPassDesc* pBeginDesc) override final;
		virtual void EndRenderPass() override final;

		virtual void BuildTopLevelAccelerationStructure(const BuildTopLevelAccelerationStructureDesc* pBuildDesc)		override final;
		virtual void BuildBottomLevelAccelerationStructure(const BuildBottomLevelAccelerationStructureDesc* pBuildDesc)	override final;

		virtual void CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes)	override final;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc)      override final;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount)	override final;
		virtual void PipelineBufferBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineBufferBarrier* pBufferBarriers, uint32 bufferBarrierCount)		override final;

		virtual void GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter) override final;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)           override final;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)    override final;

		virtual void SetConstantRange(const IPipelineLayout* pPipelineLayout, uint32 shaderStageMask, const void* pConstants, uint32 size, uint32 offset) override final;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer, uint64 offset) override;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) override final;

		virtual void BindDescriptorSetGraphics(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) override final;
		virtual void BindDescriptorSetCompute(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) override final;
		virtual void BindDescriptorSetRayTracing(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) override final;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)		override final;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		override final;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	override final;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset) override final;

		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) override final;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)                          override final;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)  override final;
		virtual void DrawIndexedIndirect(const IBuffer* pDrawBuffer, uint32 offset, uint32 drawCount, uint32 stride)							override final;

		virtual void ExecuteSecondary(const ICommandList* pSecondary) override final;

		virtual ICommandAllocator* GetAllocator() const override final;

		FORCEINLINE virtual CommandListDesc GetDesc() const override final
		{
			return m_Desc;
		}

		FORCEINLINE virtual uint64 GetHandle() const override final
		{
			return (uint64)m_CommandList;
		}

		
		FORCEINLINE virtual ECommandQueueType GetType()	const override final
		{
			return m_Type;
		}
	

	private:
		void BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout, VkPipelineBindPoint bindPoint);

	private:
		VkCommandBuffer						m_CommandList					= VK_NULL_HANDLE;
		CommandAllocatorVK*                 m_pAllocator	                = nullptr;
        const RayTracingPipelineStateVK*    m_pCurrentRayTracingPipeline    = nullptr;
        
        VkImageMemoryBarrier    m_ImageBarriers[MAX_IMAGE_BARRIERS];
		VkBufferMemoryBarrier   m_BufferBarriers[MAX_BUFFER_BARRIERS];
        VkViewport              m_Viewports[MAX_VIEWPORTS];
        VkRect2D                m_ScissorRects[MAX_VIEWPORTS];
        VkBuffer                m_VertexBuffers[MAX_VERTEX_BUFFERS];
        VkDeviceSize            m_VertexBufferOffsets[MAX_VERTEX_BUFFERS];
		VkClearValue			m_ClearValues[MAX_RENDERTARGETS+1];
        
        CommandListDesc			m_Desc;
		ECommandQueueType		m_Type;
	};
}
