#include "Rendering/PaintMaskRenderer.h"

namespace LambdaEngine
{
	PaintMaskRenderer::PaintMaskRenderer()
	{
	}

	PaintMaskRenderer::~PaintMaskRenderer()
	{
	}

	bool PaintMaskRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		return false;
	}

	void PaintMaskRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
	}

	void PaintMaskRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
	}

	void PaintMaskRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
	}

	void PaintMaskRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		//Kallas när ett DrawArg har uppdaterats i RenderGraphen, du kan anta att DrawArgMasken överenstämmer med det som är angivet i RenderGraphen 
	}

	void PaintMaskRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage)
	{
	}
}