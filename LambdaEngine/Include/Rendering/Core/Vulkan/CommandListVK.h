#pragma once
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/DeviceChildBase.h"

#include "Vulkan.h"

#define MAX_IMAGE_BARRIERS 8

namespace LambdaEngine
{
	class GraphicsDeviceVK;
	class CommandAllocatorVK;

	class CommandListVK : public DeviceChildBase<GraphicsDeviceVK, ICommandList>
	{
		using TDeviceChild = DeviceChildBase<GraphicsDeviceVK, ICommandList>;

	public:
		CommandListVK(const GraphicsDeviceVK* pDevice);
		~CommandListVK();

		bool Init(ICommandAllocator* pAllocator, const CommandListDesc& desc);

		virtual void Begin(const SecondaryCommandListBeginDesc* pBeginDesc)	override;
		
		virtual void Reset()	override;
		virtual void End()		override;

		virtual void BeginRenderPass(const IRenderPass* pRenderPass, const IFrameBuffer* pFrameBuffer, uint32 width, uint32 height, uint32 flags) override;
		virtual void EndRenderPass() override;

		virtual void BuildTopLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)		override;
		virtual void BuildBottomLevelAccelerationStructure(IBottomLevelAccelerationStructure* pAccelerationStructure)	override;

		virtual void CopyBuffer(const IBuffer* pSrc, uint64 srcOffset, IBuffer* pDst, uint64 dstOffset, uint64 sizeInBytes)	override;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pDst, const CopyTextureFromBufferDesc& desc) override;

		virtual void PipelineTextureBarriers(FPipelineStageFlags srcStage, FPipelineStageFlags dstStage, const PipelineTextureBarrier* pTextureBarriers, uint32 textureBarrierCount) override;

		virtual void GenerateMiplevels(ITexture* pTexture, ETextureState stateBefore, ETextureState stateAfter) override;

		virtual void SetViewports(const Viewport* pViewports) override;
		virtual void SetScissorRects(const ScissorRect* pScissorRects) override;

		virtual void SetConstantGraphics()	override;
		virtual void SetConstantCompute()	override;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer) override;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, const uint32* pOffsets, uint32 vertexBufferCount) override;

		virtual void BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) override;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)		override;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		override;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	override;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset) override;
		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ) override;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) override;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance) override;

		virtual void ExecuteSecondary(const ICommandList* pSecondary) override;

		virtual ICommandAllocator* GetAllocator() const override;

		FORCEINLINE virtual CommandListDesc GetDesc() const override
		{
			return m_Desc;
		}

		FORCEINLINE virtual uint64 GetHandle() const override
		{
			return (uint64)m_CommandList;
		}

	private:
		CommandAllocatorVK* m_pAllocator	= nullptr;
		VkCommandBuffer		m_CommandList	= VK_NULL_HANDLE;
		CommandListDesc		m_Desc;

		VkImageMemoryBarrier m_ImageBarriers[MAX_IMAGE_BARRIERS];
	};
}