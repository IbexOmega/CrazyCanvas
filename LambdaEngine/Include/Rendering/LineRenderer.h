#pragma once

#include "RenderGraphTypes.h"
#include "CustomRenderer.h"

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

	struct VertexData
	{
		glm::vec4 Position;
		glm::vec4 Color;
	};

	class LineRenderer : public CustomRenderer
	{
	public:
		LineRenderer(const GraphicsDevice* pGraphicsDevice, uint32 verticiesBufferSize, uint32 backBufferCount);
		virtual ~LineRenderer();

		virtual bool Init() override final;

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;

		virtual void UpdateTextureResource(
			const String& resourceName,
			const TextureView* const* ppPerImageTextureViews,
			const TextureView* const* ppPerSubImageTextureViews,
			uint32 imageCount,
			uint32 subImageCount,
			bool backBufferBound) override final;

		virtual void UpdateBufferResource(const String& resourceName,
			const Buffer* const* ppBuffers,
			uint64* pOffsets,
			uint64* pSizesInBytes,
			uint32 count,
			bool backBufferBound) override final;

		virtual void Render(uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool Sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }
		FORCEINLINE virtual const String& GetName() const override
		{
			static String name = RENDER_GRAPH_LINE_RENDERER_STAGE_NAME;
			return name;
		}

		/*
		* Adds a line group to be rendered. The number of lines is equal to points.GetSize() / 2
		*	points - Array of points to be added to renderer
		*	color - Color the lines should be
		*	return - Returns an ID to be used when removing or updating the set of points using Update- or RemoveLineGroup
		*/
		static uint32 AddLineGroup(const TArray<glm::vec3>& points, const glm::vec3& color);

		/*
		* Update a previously added group using the ID of the points - If ID does not exist, uses AddLineGroup
		*	ID - Identification of the set of points to be updated
		*	points - Array of points to be updated
		*	color - Color the lines should be
		*	return - Returns an ID to be used when removing or updating the set of points using Update- or RemoveLineGroup
		*/
		static uint32 UpdateLineGroup(uint32 ID, const TArray<glm::vec3>& points, const glm::vec3& color);

		/*
		* Remove a previously added group using the ID of the points
		*	ID - Identification of the set of points to be removed
		*/
		static void RemoveLineGroup(uint32 ID);

		/*
		* Draw a line that will be static in the scene and cannot be removed (legacy Bullet implementation)
		* from - glm::vec3 of point to draw from
		* to - glm::vec3 of point to draw to
		* color - color the line between from and to should be
		*/
		void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color);

		/*
		* Draw a line that will be static in the scene and cannot be removed (legacy Bullet implementation)
		* from - glm::vec3 of point to draw from
		* to - glm::vec3 of point to draw to
		* fromColor - start color the line should be - will be interpolated to the toColor
		* toColor -  end color the line should be - will be interpolated from fromColor
		*/
		void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& fromColor, const glm::vec3& toColor);

		/*
		* Sets the line width to be used for all lines
		* lineWidth - width of the lines
		*/
		static void SetLineWidth(float32 lineWidth);

	public:
		static LineRenderer* Get();

	private:
		bool CreateCopyCommandList();
		bool CreateBuffers(uint32 verticiesBufferSize);
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		const GraphicsDevice*	m_pGraphicsDevice = nullptr;

		uint32 m_verticiesBufferSize;

		uint32 m_BackBufferCount = 0;
		TArray<TSharedRef<const TextureView>>	m_BackBuffers;
		TSharedRef<const TextureView>			m_DepthStencilBuffer;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator	= nullptr;
		TSharedRef<CommandList>			m_CopyCommandList		= nullptr;

		CommandAllocator**	m_ppRenderCommandAllocators	= nullptr;
		CommandList**		m_ppRenderCommandLists		= nullptr;

		uint64						m_PipelineStateID	= 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout	= nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap	= nullptr;
		TSharedRef<DescriptorSet>	m_DescriptorSet		= nullptr;

		GUID_Lambda m_VertexShaderGUID	= 0;
		GUID_Lambda m_PixelShaderGUID	= 0;

		TSharedRef<RenderPass> m_RenderPass = nullptr;

		TArray<TSharedRef<Buffer>> m_UniformCopyBuffers;
		TSharedRef<Buffer> m_UniformBuffer	= nullptr;

		THashTable<String, TArray<TSharedRef<DescriptorSet>>>		m_BufferResourceNameDescriptorSetsMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;

	private:
		static LineRenderer*							s_pInstance;
		static TArray<VertexData>						s_Verticies;
		static THashTable<uint32, TArray<VertexData>>	s_LineGroups;
		static float32									s_LineWidth;
	};

}