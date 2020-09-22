#pragma once

#include <bullet/src/btBulletDynamicsCommon.h>
#include "RenderGraphTypes.h"
#include "ICustomRenderer.h"

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

	/*
	* ImGuiRendererDesc
	*/
	struct PhysicsRendererDesc
	{
		uint32 BackBufferCount = 0;
		uint32 VertexBufferSize = 0;
		uint32 IndexBufferSize = 0;
	};

	struct VertexData
	{
		glm::vec4 Position;
		glm::vec4 Color;
	};

	class PhysicsRenderer : public btIDebugDraw, ICustomRenderer
	{
	public:
		PhysicsRenderer();
		virtual ~PhysicsRenderer();

		bool init(const PhysicsRendererDesc* pDesc);

		// Custom Renderer implementations
		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		virtual void PreBuffersDescriptorSetWrite() override final;
		virtual void PreTexturesDescriptorSetWrite() override final;
		virtual void UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound) override final;
		virtual void UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound) override final;
		virtual void UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure) override final;
		virtual void Render(
			CommandAllocator* pGraphicsCommandAllocator,
			CommandList* pGraphicsCommandList,
			CommandAllocator* pComputeCommandAllocator,
			CommandList* pComputeCommandList,
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppPrimaryExecutionStage,
			CommandList** ppSecondaryExecutionStage)	override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_INPUT; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage()	override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }
		FORCEINLINE virtual const String& GetName() const  override
		{
			static String name = RENDER_GRAPH_IMGUI_STAGE_NAME;
			return name;
		}


		// DebugDraw implementations
		virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override final;
		virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor) override final;
		//virtual void drawSphere(const btVector3& p, btScalar radius, const btVector3& color) override final;
		//virtual void drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha) override final;
		virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override final;
		virtual void reportErrorWarning(const char* warningString) override final;
		virtual void draw3dText(const btVector3& location, const char* textString) override final;
		virtual void setDebugMode(int debugMode) override final { m_DebugMode = debugMode; }
		virtual int getDebugMode() const override final { return m_DebugMode; }

	private:
		bool CreateCopyCommandList();
		bool CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize);
		//bool CreateTextures();
		//bool CreateSamplers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc);
		bool CreatePipelineState();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader);

	private:
		int m_DebugMode = 0;

		const GraphicsDevice*	m_pGraphicsDevice = nullptr;

		TArray<VertexData> m_Verticies;

		TArray<TSharedRef<const TextureView>>	m_BackBuffers;

		TSharedRef<CommandAllocator>	m_CopyCommandAllocator	= nullptr;
		TSharedRef<CommandList>			m_CopyCommandList		= nullptr;

		uint64						m_PipelineStateID	= 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout	= nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap	= nullptr;
		TSharedRef<DescriptorSet>	m_DescriptorSet		= nullptr;

		GUID_Lambda m_VertexShaderGUID	= 0;
		GUID_Lambda m_PixelShaderGUID	= 0;

		TSharedRef<RenderPass> m_RenderPass = nullptr;

		// TArray<TSharedRef<Buffer>> m_VertexCopyBuffers;
		TArray<TSharedRef<Buffer>> m_UniformCopyBuffers;
		TArray<TSharedRef<Buffer>> m_IndexCopyBuffers;
		// TSharedRef<Buffer> m_VertexBuffer	= nullptr;
		TSharedRef<Buffer> m_IndexBuffer	= nullptr;
		TSharedRef<Buffer> m_UniformBuffer	= nullptr;

		TSharedRef<Texture>		m_FontTexture		= nullptr;
		TSharedRef<TextureView>	m_FontTextureView	= nullptr;

		TSharedRef<Sampler> m_Sampler = nullptr;

		THashTable<String, TArray<TSharedRef<DescriptorSet>>>		m_BufferResourceNameDescriptorSetsMap;
		THashTable<GUID_Lambda, THashTable<GUID_Lambda, uint64>>	m_ShadersIDToPipelineStateIDMap;
	};

}