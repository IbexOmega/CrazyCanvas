#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/String.h"

#include "RenderGraphTypes.h"

#include "Utilities/StringHash.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	struct PipelineTextureBarrierDesc;
	struct PipelineBufferBarrierDesc;
	struct TextureViewDesc;
	struct TextureDesc;
	struct SamplerDesc;
	struct BufferDesc;

	class IAccelerationStructure;
	class ICommandAllocator;
	class IGraphicsDevice;
	class IPipelineLayout;
	class IPipelineState;
	class IDescriptorSet;
	class IDescriptorHeap;
	class ICommandList;
	class ITextureView;
	class ITexture;
	class ISampler;
	class IBuffer;
	class IFence;
	class Scene;

	struct RenderGraphDesc
	{
		const char* pName						= "Render Graph";
		bool CreateDebugStages					= false;
		bool CreateDebugGraph					= false;
		RenderStageDesc* pRenderStages			= nullptr;
		uint32 RenderStageCount					= 0;
		uint32 BackBufferCount					= 3;
		uint32 MaxTexturesPerDescriptorSet		= 1;
		const Scene* pScene						= nullptr;
	};

	struct ResourceUpdateDesc
	{
		const char* pResourceName	= "No Resource Name";

		union
		{
			struct
			{
				void*				pData;
				uint32				DataSize;
			} PushConstantUpdate;

			struct
			{
				TextureDesc**		ppTextureDesc;
				TextureViewDesc**	ppTextureViewDesc;
				SamplerDesc**		ppSamplerDesc;
			} InternalTextureUpdate;

			struct
			{
				BufferDesc**		ppBufferDesc;
			} InternalBufferUpdate;

			struct
			{
				ITexture**			ppTextures;
				ITextureView**		ppTextureViews;
				ISampler**			ppSamplers;
			} ExternalTextureUpdate;

			struct
			{
				IBuffer**			ppBuffer;
			} ExternalBufferUpdate;

			struct
			{
				const IAccelerationStructure* pTLAS;
			} ExternalAccelerationStructure;
		};
	};

	struct RenderStageParameters
	{
		const char* pRenderStageName	= "No Render Stage Name";

		union
		{
			struct
			{
				uint32 Width;
				uint32 Height;
			} Graphics;

			struct
			{
				uint32 WorkGroupCountX;
				uint32 WorkGroupCountY;
				uint32 WorkGroupCountZ;
			} Compute;

			struct
			{
				uint32 RayTraceWidth;
				uint32 RayTraceHeight;
				uint32 RayTraceDepth;
			} RayTracing;

			struct
			{
				void* pUserData;
			} Custom;
		};
	};

	struct MaterialBindingInfo
	{
		uint32	Stride;
	};

	class LAMBDA_API RenderGraph
	{
		static constexpr const char* RENDER_GRAPH_BACK_BUFFER_PROXY = "BACK_BUFFER_PROXY_TEXTURE";

		enum class EResourceType
		{
			UNKNOWN								= 0,
			PUSH_CONSTANTS						= 1,
			INTERNAL_TEXTURE					= 2,
			INTERNAL_BUFFER						= 3,
			EXTERNAL_TEXTURE					= 4,
			EXTERNAL_BUFFER						= 5,
			EXTERNAL_ACCELERATION_STRUCTURE		= 6,
		};

		struct RenderStage;

		struct ResourceBinding
		{
			RenderStage*	pRenderStage	= nullptr;
			EDescriptorType DescriptorType	= EDescriptorType::DESCRIPTOR_UNKNOWN;
			uint32			Binding			= 0;

			ETextureState TextureState		= ETextureState::TEXTURE_STATE_UNKNOWN;
		};

		struct Resource
		{
			const char*		pName				= "";
			EResourceType	Type				= EResourceType::UNKNOWN;
			uint32			SubResourceCount	= 0;

			std::vector<ResourceBinding>	ResourceBindings;

			struct 
			{
				void*			pData;
				uint32			DataSize;
			} PushConstants;

			struct
			{
				TArray<uint32>			Barriers; //Divided into #SubResourceCount Barriers per Synchronization Stage
				TArray<ITexture*>		Textures;
				TArray<ITextureView*>	TextureViews;
				TArray<ISampler*>		Samplers;
			} Texture;

			struct
			{
				TArray<uint32>			Barriers;
				TArray<IBuffer*>		Buffers;
				TArray<uint64>			Offsets;
				TArray<uint64>			SizesInBytes;
			} Buffer;

			struct
			{
				const IAccelerationStructure* pTLAS;
			} AccelerationStructure;
		};

		struct RenderStage
		{
			EPipelineStateType		PipelineStateType				= EPipelineStateType::NONE;
			RenderStageParameters	Parameters						= {};

			bool					UsesCustomRenderer				= false;
			ICustomRenderer*		pCustomRenderer					= nullptr;

			FPipelineStageFlags		FirstPipelineStage				= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags		LastPipelineStage				= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;

			ERenderStageDrawType	DrawType						= ERenderStageDrawType::NONE;
			Resource*				pVertexBufferResource			= nullptr;
			Resource*				pIndexBufferResource			= nullptr;
			Resource*				pMeshIndexBufferResource		= nullptr;

			uint64					PipelineStateID					= 0;
			IPipelineLayout*		pPipelineLayout					= nullptr;
			uint32					TextureSubDescriptorSetCount	= 1;
			uint32					MaterialsRenderedPerPass		= 1;
			IDescriptorSet**		ppTextureDescriptorSets			= nullptr; //# m_BackBufferCount * ceil(# Textures per Draw / m_MaxTexturesPerDescriptorSet)
			IDescriptorSet**		ppBufferDescriptorSets			= nullptr; //# m_BackBufferCount
			IRenderPass*			pRenderPass						= nullptr;

			Resource*				pPushConstantsResource			= nullptr;
			TArray<Resource*>		RenderTargetResources;
			Resource*				pDepthStencilAttachment			= nullptr;
		};

		struct TextureSynchronization
		{
			FShaderStageFlags		SrcShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			uint32					BarrierUseFrameIndex	= 0;
			uint32					SameFrameBarrierOffset	= 1;
			TArray<uint32>			Barriers;
		};

		struct BufferSynchronization
		{
			FShaderStageFlags		SrcShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			uint32					BarrierUseFrameIndex	= 0;
			uint32					SameFrameBarrierOffset	= 1;
			TArray<uint32>			Barriers;
		};

		struct SynchronizationStage
		{
			THashTable<std::string, TextureSynchronization> TextureSynchronizations;
			THashTable<std::string, BufferSynchronization> BufferSynchronizations;
		};

		struct PipelineStage
		{
			EPipelineStageType	Type			= EPipelineStageType::NONE;
			uint32				StageIndex		= 0;

			ICommandAllocator** ppGraphicsCommandAllocators;
			ICommandAllocator** ppComputeCommandAllocators;
			ICommandList** ppGraphicsCommandLists;
			ICommandList** ppComputeCommandLists;
		};

	public:
		DECL_REMOVE_COPY(RenderGraph);
		DECL_REMOVE_MOVE(RenderGraph);

		RenderGraph(const IGraphicsDevice* pGraphicsDevice);
		~RenderGraph();

		bool Init(const RenderGraphDesc* pDesc);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc& desc);

		void UpdateRenderStageParameters(const RenderStageParameters& desc);

		void GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets, the RenderGraph will wait for device idle if it needs to
		*/
		void Update();

		void NewFrame(Timestamp delta);
		void PrepareRender(Timestamp delta);

		void Render(uint64 frameIndex, uint32 backBufferIndex);

		bool GetResourceTextures(const char* pResourceName, ITexture* const ** pppTexture, uint32* pTextureView)						const;
		bool GetResourceTextureViews(const char* pResourceName, ITextureView* const ** pppTextureViews, uint32* pTextureViewCount)		const;
		bool GetResourceBuffers(const char* pResourceName, IBuffer* const ** pppBuffers, uint32* pBufferCount)							const;
		bool GetResourceAccelerationStructure(const char* pResourceName, const IAccelerationStructure** ppAccelerationStructure)		const;

	private:
		bool CreateFence();
		bool CreateDescriptorHeap();
		bool CreateCopyCommandLists();
		bool CreateResources(const std::vector<RenderStageResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions);
		bool CreateSynchronizationStages(const std::vector<SynchronizationStageDesc>& synchronizationStageDescriptions);
		bool CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions);

		void UpdateResourcePushConstants(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalTexture(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalBuffer(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalTexture(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalBuffer(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalAccelerationStructure(Resource* pResource, const ResourceUpdateDesc& desc);

		void ExecuteSynchronizationStage(
			SynchronizationStage* pSynchronizationStage, 
			ICommandAllocator* pGraphicsCommandAllocator, 
			ICommandList* pGraphicsCommandList, 
			ICommandAllocator* pComputeCommandAllocator, 
			ICommandList* pComputeCommandList, 
			ICommandList** ppFirstExecutionStage, 
			ICommandList** ppSecondExecutionStage);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, IPipelineState* pPipelineState, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList, ICommandList** ppExecutionStage);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, IPipelineState* pPipelineState, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, IPipelineState* pPipelineState, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		const Scene*										m_pScene							= nullptr;

		IDescriptorHeap*									m_pDescriptorHeap					= nullptr;

		uint64												m_ModFrameIndex						= 0;
		uint32												m_BackBufferIndex					= 0;
		uint32												m_BackBufferCount					= 0;
		uint32												m_MaxTexturesPerDescriptorSet		= 0;
		
		IFence*												m_pFence							= nullptr;
		uint64												m_SignalValue						= 1;

		TArray<ICustomRenderer*>							m_CustomRenderers;
		TArray<ICustomRenderer*>							m_DebugRenderers;

		ICommandAllocator**									m_ppGraphicsCopyCommandAllocators	= nullptr;
		ICommandList**										m_ppGraphicsCopyCommandLists		= nullptr;
		bool												m_ExecuteGraphicsCopy				= false;

		ICommandAllocator**									m_ppComputeCopyCommandAllocators	= nullptr;
		ICommandList**										m_ppComputeCopyCommandLists			= nullptr;
		bool												m_ExecuteComputeCopy				= false;

		ICommandList**										m_ppExecutionStages					= nullptr;
		uint32												m_ExecutionStageCount				= 0;

		PipelineStage*										m_pPipelineStages					= nullptr;
		uint32												m_PipelineStageCount				= 0;

		THashTable<String, uint32>							m_RenderStageMap;
		RenderStage*										m_pRenderStages						= nullptr;
		uint32												m_RenderStageCount					= 0;

		SynchronizationStage*								m_pSynchronizationStages			= nullptr;
		uint32												m_SynchronizationStageCount			= 0;

		THashTable<String, Resource>						m_ResourceMap;
		TSet<Resource*>										m_DirtyDescriptorSetInternalTextures;
		TSet<Resource*>										m_DirtyDescriptorSetInternalBuffers;
		TSet<Resource*>										m_DirtyDescriptorSetExternalTextures;
		TSet<Resource*>										m_DirtyDescriptorSetExternalBuffers;
		TSet<Resource*>										m_DirtyDescriptorSetAccelerationStructures;

		TArray<PipelineTextureBarrierDesc>					m_TextureBarriers;
		TArray<PipelineBufferBarrierDesc>					m_BufferBarriers;
	};
}
