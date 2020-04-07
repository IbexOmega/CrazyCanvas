#pragma once
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

#define MAX_IMAGE_BARRIERS  8
#define MAX_VIEWS           32
#define MAX_VERTEX_BUFFERS  32

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class CommandAllocatorVK;
    class RayTracingPipelineStateVK;

	class CommandListVK : public DeviceChildBase<GraphicsDeviceVK, ICommandList>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ICommandList>;

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
		
		virtual void Reset()	override final;
		virtual void End()		override final;

		virtual void BeginRenderPass(const IRenderPass* pRenderPass, const IFrameBuffer* pFrameBuffer, uint32 width, uint32 height, uint32 flags) override final;
		virtual void EndRenderPass() override final;

		virtual void BuildTopLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)		override final;
		virtual void BuildBottomLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)	override final;

		virtual void CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes)	override final;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc)      override final;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount) override final;

		virtual void GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter) override final;

		virtual void SetViewports(const Viewport* pViewports, uint32 firstViewport, uint32 viewportCount)           override final;
		virtual void SetScissorRects(const ScissorRect* pScissorRects, uint32 firstScissor, uint32 scissorCount)    override final;

		virtual void SetConstantGraphics()	override final;
		virtual void SetConstantCompute()	override final;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer, uint64 offset) override;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, uint32 firstBuffer, const uint64* pOffsets, uint32 vertexBufferCount) override final;

		virtual void BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) override final;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)		override final;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		override final;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	override final;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset) override final;
        
		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) override final;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)                          override final;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)  override final;

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

	private:
		CommandAllocatorVK*                 m_pAllocator	                = nullptr;
        const RayTracingPipelineStateVK*    m_pCurrentRayTracingPipeline    = nullptr;
        
		VkCommandBuffer	m_CommandList = VK_NULL_HANDLE;

        VkImageMemoryBarrier    m_ImageBarriers[MAX_IMAGE_BARRIERS];
        VkViewport              m_Viewports[MAX_VIEWS];
        VkRect2D                m_ScissorRects[MAX_VIEWS];
        VkBuffer                m_VertexBuffers[MAX_VERTEX_BUFFERS];
        VkDeviceSize            m_VertexBufferOffsets[MAX_VERTEX_BUFFERS];
        
        CommandListDesc m_Desc;
	};
}
