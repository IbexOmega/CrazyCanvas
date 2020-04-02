#pragma once
#include "IDeviceChild.h"
#include "GraphicsTypes.h"

namespace LambdaEngine
{
	class IBuffer;
	class ITexture;
	class IRenderPass;
	class ITextureView;
	class IPipelineState;
	class IDescriptorSet;
	class IPipelineLayout;

	class ICommandList : public IDeviceChild
	{
	public:
		DECL_INTERFACE(ICommandList);

		virtual void Reset()	= 0;
		virtual void Begin()	= 0;
		virtual void End()		= 0;

		virtual void BeginRenderPass(const IRenderPass* pRenderPass, const ITextureView* const * ppRenderTargets) = 0;
		virtual void EndRenderPass() = 0;

		virtual void CopyBuffer(const IBuffer* pSrc, IBuffer* pDst)					= 0;
		virtual void CopyTextureFromBuffer(const IBuffer* pSrc, ITexture* pTexture)	= 0;

		virtual void GenerateMiplevels(ITexture* pTexture) = 0;

		virtual void SetViewports(const Viewport* pViewports)			= 0;
		virtual void SetScissorRects(const ScissorRect* pScissorRects)	= 0;
		
		virtual void SetConstantGraphics()	= 0;
		virtual void SetConstantCompute()	= 0;

		virtual void BindIndexBuffer(const IBuffer* pIndexBuffer) = 0;
		virtual void BindVertexBuffers(const IBuffer* const* ppVertexBuffers, const uint32* pOffsets, uint32 vertexBufferCount) = 0;

		virtual void BindDescriptorSet(const IDescriptorSet* pDescriptorSet, const IPipelineLayout* pPipelineLayout) = 0;

		virtual void BindGraphicsPipeline(const IPipelineState* pPipeline)	= 0;
		virtual void BindComputePipeline(const IPipelineState* pPipeline)		= 0;
		virtual void BindRayTracingPipeline(const IPipelineState* pPipeline)	= 0;

		virtual void TraceRays(uint32 width, uint32 height, uint32 raygenOffset)						= 0;
		virtual void Dispatch(uint32 workGroupCountX, uint32 workGroupCountY, uint32 workGroupCountZ)	= 0;

		virtual void DrawInstanced(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance)							= 0;
		virtual void DrawIndexInstanced(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, uint32 vertexOffset, uint32 firstInstance)	= 0;

		virtual void ExecuteSecondary(const ICommandList* pSecondary) = 0;

		virtual ECommandListType GetType() = 0;
	};
}