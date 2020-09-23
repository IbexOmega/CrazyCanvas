#include "GUI/GUIRenderer.h"

#include "GUI/GUIRenderTarget.h"
#include "GUI/GUITexture.h"

namespace LambdaEngine
{
	GUIRenderer::GUIRenderer()
	{
	}
	GUIRenderer::~GUIRenderer()
	{
	}
	bool GUIRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		return true;
	}
	
	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CreateRenderTarget(const char* label, uint32_t width, uint32_t height, uint32_t sampleCount)
	{
		

		return Noesis::Ptr<Noesis::RenderTarget>();
	}

	Noesis::Ptr<Noesis::RenderTarget> GUIRenderer::CloneRenderTarget(const char* label, Noesis::RenderTarget* surface)
	{
		return Noesis::Ptr<Noesis::RenderTarget>();
	}

	Noesis::Ptr<Noesis::Texture> GUIRenderer::CreateTexture(const char* label, uint32_t width, uint32_t height, uint32_t numLevels, Noesis::TextureFormat::Enum format, const void** data)
	{
		return Noesis::Ptr<Noesis::Texture>();
	}

	void GUIRenderer::UpdateTexture(Noesis::Texture* texture, uint32_t level, uint32_t x, uint32_t y, uint32_t width, uint32_t height, const void* data)
	{
	}

	void GUIRenderer::BeginRender(bool offscreen)
	{
	}

	void GUIRenderer::SetRenderTarget(Noesis::RenderTarget* surface)
	{
	}

	void GUIRenderer::BeginTile(const Noesis::Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight)
	{
	}

	void GUIRenderer::EndTile()
	{
	}

	void GUIRenderer::ResolveRenderTarget(Noesis::RenderTarget* surface, const Noesis::Tile* tiles, uint32_t numTiles)
	{
	}

	void GUIRenderer::EndRender()
	{
	}

	void* GUIRenderer::MapVertices(uint32_t bytes)
	{
		return nullptr;
	}

	void GUIRenderer::UnmapVertices()
	{
	}

	void* GUIRenderer::MapIndices(uint32_t bytes)
	{
		return nullptr;
	}

	void GUIRenderer::UnmapIndices()
	{
	}

	void GUIRenderer::DrawBatch(const Noesis::Batch& batch)
	{
	}

	void GUIRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void GUIRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void GUIRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
	}

	void GUIRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
	}

	void GUIRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
	}

	void GUIRenderer::Render(CommandAllocator* pGraphicsCommandAllocator, CommandList* pGraphicsCommandList, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppPrimaryExecutionStage, CommandList** ppSecondaryExecutionStage)
	{
	}
}