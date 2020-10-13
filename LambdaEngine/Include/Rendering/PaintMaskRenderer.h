#pragma once

#include "Rendering/ICustomRenderer.h"
#include "Rendering/RenderGraph.h"
#include <optional>

namespace LambdaEngine
{
	class CommandAllocator;
	class DeviceAllocator;
	class GraphicsDevice;
	class PipelineLayout;
	class DescriptorHeap;
	class DescriptorSet;
	class PipelineState;
	class CommandList;
	class TextureView;
	class CommandList;
	class RenderPass;
	class Texture;
	class Sampler;
	class Shader;
	class Buffer;
	class Window;

	enum class EPaintMode
	{
		REMOVE = 0,
		PAINT
	};

	class PaintMaskRenderer : public ICustomRenderer
	{
	public:
		PaintMaskRenderer();
		~PaintMaskRenderer();

		bool init(GraphicsDevice* pGraphicsDevice, uint32 backBufferCount);

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		virtual void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex) override final;
		virtual void PreBuffersDescriptorSetWrite() override final;
		virtual void PreTexturesDescriptorSetWrite() override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const * ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const * ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;
		virtual void UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count) override final;
		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_MESH_UNWRAP_NAME;
			return name;
		}

	public:
		/* Adds a hitpoint to draw out a splash at
		* Note: Currently only one hitpoint will be handled at each frame
		*	position - vec3 of the hit point position
		*	direction - vec3 of the direction the hit position had during collision
		*	paintMode - painting mode to be used for the target
		*/
		static void AddHitPoint(const glm::vec3& position, const glm::vec3& direction, EPaintMode paintMode);

	private:
		bool CreateCopyCommandList();
		bool CreateBuffers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(const CustomRendererRenderGraphInitDesc* pPreInitDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		struct RenderTarget
		{
			TextureView*	TextureView		= nullptr;
			uint32			DrawArgIndex	= 0;
			uint32			InstanceIndex	= 0;
		};

		struct UnwrapData
		{
			glm::vec4	TargetPosition;
			glm::vec4	TargetDirection;
			EPaintMode	PaintMode = EPaintMode::PAINT;
		};

	private:
		const GraphicsDevice* m_pGraphicsDevice = nullptr;

		uint32 m_BackBufferCount = 0;
		TArray<TSharedRef<const TextureView>>	m_BackBuffers;
		TSharedRef<const TextureView>			m_DepthStencilBuffer;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator = nullptr;
		TSharedRef<CommandList>			m_CopyCommandList = nullptr;

		CommandAllocator**			m_ppRenderCommandAllocators = nullptr;
		CommandList**				m_ppRenderCommandLists = nullptr;

		uint64						m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout = nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap = nullptr;

		GUID_Lambda					m_VertexShaderGUID = 0;
		GUID_Lambda					m_PixelShaderGUID = 0;

		TSharedRef<RenderPass>		m_RenderPass = nullptr;

		TSharedRef<Sampler>			m_Sampler = nullptr;
		TArray<TSharedRef<Buffer>>	m_UnwrapDataCopyBuffers;
		TSharedRef<Buffer>			m_UnwrapDataBuffer = nullptr;

		const DrawArg*												m_pDrawArgs = nullptr;
		TArray<TArray<TSharedRef<DescriptorSet>>>					m_VerticesInstanceDescriptorSets;

		TSharedRef<DescriptorSet>									m_UnwrapDataDescriptorSet;
		TSharedRef<DescriptorSet>									m_PerFrameBufferDescriptorSet;
		TSharedRef<DescriptorSet>									m_BrushMaskDescriptorSet;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;

		TArray<TArray<TSharedRef<DeviceChild>>>						m_pDeviceResourcesToDestroy;

		TArray<RenderTarget>										m_RenderTargets;

	private:
		static std::list<UnwrapData>	s_Collisions;
	};
}