#pragma once

#include "Rendering/CustomRenderer.h"
#include "Rendering/RenderGraph.h"
#include <optional>

#define MAX_PAINT_PER_FRAME 10

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
		REMOVE	= 0,
		PAINT	= 1,
		NONE	= 2
	};

	enum class ERemoteMode
	{
		UNDEFINED 	= 0,
		CLIENT		= 1,
		SERVER		= 2
	};

	enum class ETeam
	{
		NONE	= 0,
		RED		= 1,
		BLUE	= 2
	};

	class PaintMaskRenderer : public CustomRenderer
	{
	public:
		PaintMaskRenderer(GraphicsDevice* pGraphicsDevice, uint32 backBufferCount);
		~PaintMaskRenderer();

		virtual bool Init() override final;

		virtual bool RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc) override final;
		
		virtual void UpdateTextureResource(
			const String& resourceName, 
			const TextureView* const * ppPerImageTextureViews, 
			const TextureView* const* ppPerSubImageTextureViews, 
			uint32 imageCount, 
			uint32 subImageCount, 
			bool backBufferBound) override final;

		virtual void UpdateBufferResource(
			const String& resourceName, 
			const Buffer* const * ppBuffers, 
			uint64* pOffsets, 
			uint64* pSizesInBytes, 
			uint32 count, 
			bool backBufferBound) override final;

		virtual void UpdateDrawArgsResource(
			const String& resourceName, 
			const DrawArg* pDrawArgs, 
			uint32 count) override final;
		
		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_VERTEX_SHADER; }
		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final { return FPipelineStageFlag::PIPELINE_STAGE_FLAG_PIXEL_SHADER; }

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = RENDER_GRAPH_MESH_UNWRAP_NAME;
			return name;
		}

	public:
		/* Adds a hitpoint to draw out a splash at
		*	position - vec3 of the hit point position
		*	direction - vec3 of the direction the hit position had during collision
		*	paintMode - painting mode to be used for the target
		*/
		static void AddHitPoint(const glm::vec3& position, const glm::vec3& direction, EPaintMode paintMode, ERemoteMode remoteMode, ETeam mode);

		/* Reset client data from the texture and only use the verifed server data */
		static void ResetClient();

	private:
		bool CreateCopyCommandList();
		bool CreateBuffers();
		bool CreatePipelineLayout();
		bool CreateDescriptorSet();
		bool CreateShaders();
		bool CreateCommandLists();
		bool CreateRenderPass(const CustomRendererRenderGraphInitDesc* pPreInitDesc);
		bool CreatePipelineState();
		void AddResetHitPoint();

		uint64 InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader, FColorComponentFlags colorComponentFlags);

	private:
		struct RenderTarget
		{
			TextureView*	pTextureView	= nullptr;
			uint32			DrawArgIndex	= 0;
			uint32			InstanceIndex	= 0;
		};

		struct UnwrapData
		{
			glm::vec4		TargetPosition;
			glm::vec4		TargetDirection;
			EPaintMode		PaintMode			= EPaintMode::NONE;
			ERemoteMode		RemoteMode			= ERemoteMode::UNDEFINED;
			ETeam			Team				= ETeam::NONE;
			uint32			Padding0			= 0;
			bool			ClearClient			= false;
		};

		struct FrameSettings
		{
			uint32	ShouldReset	= 0;
			uint32	ShouldPaint	= 0;
			uint32	PaintCount	= 0;
			float	Angle		= 0.f;
		};

		struct DrawArgKey
		{
		public:
			DrawArgKey() = default;

			inline DrawArgKey(uint64 bufferPtr, uint64 buffer2Ptr, uint32 backBufferIndex)
				: BufferPtr(bufferPtr), Buffer2Ptr(buffer2Ptr), BackBufferIndex(backBufferIndex)
			{
				GetHash();
			}

			size_t GetHash() const
			{
				if (Hash == 0)
				{
					Hash = std::hash<uint64>()(BufferPtr);
					HashCombine<uint64>(Hash, Buffer2Ptr);
					HashCombine<uint64>(Hash, (uint64)BackBufferIndex);
				}
				return Hash;
			}

			bool operator==(const DrawArgKey& other) const
			{
				bool res = true;
				res &= BufferPtr == other.BufferPtr;
				res &= Buffer2Ptr == other.Buffer2Ptr;
				res &= BackBufferIndex == other.BackBufferIndex;
				return res;
			}

		public:
			uint64 BufferPtr;
			uint64 Buffer2Ptr;
			uint32 BackBufferIndex;
			mutable size_t	Hash = 0;
		};

		struct DrawArgKeyHasher
		{
			size_t operator()(const DrawArgKey& key) const
			{
				return key.GetHash();
			}
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

		uint64						m_PipelineStateBothID	= 0;
		uint64						m_PipelineStateClientID	= 0;
		uint64						m_PipelineStateServerID	= 0;
		TSharedRef<PipelineLayout>	m_PipelineLayout		= nullptr;
		TSharedRef<DescriptorHeap>	m_DescriptorHeap		= nullptr;

		GUID_Lambda					m_VertexShaderGUID = 0;
		GUID_Lambda					m_PixelShaderGUID = 0;

		TSharedRef<RenderPass>		m_RenderPass = nullptr;

		TSharedRef<Sampler>			m_Sampler = nullptr;
		TArray<TArray<TSharedRef<Buffer>>>	m_UnwrapDataCopyBuffers;
		TSharedRef<Buffer>			m_UnwrapDataBuffer = nullptr;

		const DrawArg*																	m_pDrawArgs = nullptr;
		TArray<TArray<DescriptorSet*>>													m_VerticesInstanceDescriptorSets;
		THashTable<DrawArgKey, TSharedRef<DescriptorSet>, DrawArgKeyHasher>				m_VerticesInstanceDescriptorSetMap;
		TArray<DrawArgKey>																m_AliveDescriptorSetList;
		TArray<DrawArgKey>																m_DeadDescriptorSetList;

		TSharedRef<DescriptorSet>														m_UnwrapDataDescriptorSet;
		TSharedRef<DescriptorSet>														m_PerFrameBufferDescriptorSet;
		TSharedRef<DescriptorSet>														m_BrushMaskDescriptorSet;

		TArray<TArray<TSharedRef<DeviceChild>>>											m_pDeviceResourcesToDestroy;

		TArray<RenderTarget>															m_RenderTargets;
	private:
		inline static bool				s_ShouldReset = false;
		static TArray<UnwrapData>		s_ServerCollisions;
		static TArray<UnwrapData>		s_ClientCollisions;
	};
}