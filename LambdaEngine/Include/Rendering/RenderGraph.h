#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"
#include "RenderGraphTypes.h"
#include "Utilities/StringHash.h"

#include <unordered_map>
#include <set>

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
		bool CreateDebugGraph					= false;
		RenderStageDesc* pRenderStages			= nullptr;
		uint32 RenderStageCount					= 0;
		uint32 BackBufferCount					= 3;
		uint32 MaxTexturesPerDescriptorSet		= 1;
		Scene* pScene							= nullptr;
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
				IAccelerationStructure* pTLAS;
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
				uint32 RaygenOffset;
				uint32 RayTraceWidth;
				uint32 RayTraceHeight;
			} RayTracing;
		};
	};

	struct MaterialBindingInfo
	{
		uint32	Stride;
	};

	class LAMBDA_API RenderGraph
	{
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
				std::vector<PipelineTextureBarrierDesc*> Barriers; //Divided into #SubResourceCount Barriers per Synchronization Stage
				std::vector<ITexture*>		Textures;
				std::vector<ITextureView*>	TextureViews;
				std::vector<ISampler*>		Samplers;
			} Texture;

			struct
			{
				std::vector<PipelineBufferBarrierDesc*> Barriers;
				std::vector<IBuffer*>		Buffers;
				std::vector<uint64>			Offsets;
				std::vector<uint64>			SizesInBytes;
			} Buffer;

			struct
			{
				IAccelerationStructure* pTLAS;
			} AccelerationStructure;
		};

		struct RenderStage
		{
			ERenderStageDrawType	DrawType						= ERenderStageDrawType::NONE;
			Resource*				pVertexBufferResource			= nullptr;
			Resource*				pIndexBufferResource			= nullptr;
			Resource*				pMeshIndexBufferResource		= nullptr;

			RenderStageParameters	Parameters						= {};
			IPipelineLayout*		pPipelineLayout					= nullptr;
			IPipelineState*			pPipelineState					= nullptr;
			uint32					TextureSubDescriptorSetCount	= 1;
			uint32					MaterialsRenderedPerPass		= 1;
			IDescriptorSet**		ppTextureDescriptorSets			= nullptr; //# m_BackBufferCount * ceil(# Textures per Draw / m_MaxTexturesPerDescriptorSet)
			IDescriptorSet**		ppBufferDescriptorSets			= nullptr; //# m_BackBufferCount
			IRenderPass*			pRenderPass						= nullptr;

			Resource*				pPushConstantsResource			= nullptr;
			std::vector<Resource*>	RenderTargetResources;
			Resource*				pDepthStencilAttachment			= nullptr;
		};

		struct TextureSynchronization
		{
			FShaderStageFlags		SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			std::vector<PipelineTextureBarrierDesc> Barriers;
		};

		struct BufferSynchronization
		{
			FShaderStageFlags		SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			std::vector<PipelineBufferBarrierDesc> Barriers;
		};

		struct SynchronizationStage
		{
			std::unordered_map<std::string, TextureSynchronization> TextureSynchronizations;
			std::unordered_map<std::string, BufferSynchronization> BufferSynchronizations;
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

		bool Init(RenderGraphDesc& desc);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc& desc);

		void UpdateRenderStageParameters(const RenderStageParameters& desc);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets, the RenderGraph will wait for device idle if it needs to
		*/
		void Update();


		/*
		* 
		*/
		void Render(uint64 frameIndex, uint32 backBufferIndex);

	private:
		bool CreateFence();
		bool CreateDescriptorHeap();
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
			ICommandList** ppSecondExecutionStage, 
			uint32 backBufferIndex);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList, ICommandList** ppExecutionStage, uint32 backBufferIndex);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage, uint32 backBufferIndex);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage, uint32 backBufferIndex);

		uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		const Scene*										m_pScene						= nullptr;

		IDescriptorHeap*									m_pDescriptorHeap				= nullptr;

		uint32												m_BackBufferCount				= 0;
		uint32												m_MaxTexturesPerDescriptorSet	= 0;
		
		IFence*												m_pFence						= nullptr;
		uint64												m_SignalValue					= 1;

		ICommandList**										m_ppExecutionStages				= nullptr;
		uint32												m_ExecutionStageCount			= 0;

		PipelineStage*										m_pPipelineStages				= nullptr;
		uint32												m_PipelineStageCount			= 0;

		std::unordered_map<std::string, uint32>				m_RenderStageMap;
		RenderStage*										m_pRenderStages					= nullptr;
		uint32												m_RenderStageCount				= 0;

		SynchronizationStage*								m_pSynchronizationStages		= nullptr;
		uint32												m_SynchronizationStageCount		= 0;

		std::unordered_map<std::string, Resource>			m_ResourceMap;
		std::set<Resource*>									m_DirtyDescriptorSetInternalTextures;
		std::set<Resource*>									m_DirtyDescriptorSetInternalBuffers;
		std::set<Resource*>									m_DirtyDescriptorSetExternalTextures;
		std::set<Resource*>									m_DirtyDescriptorSetExternalBuffers;
		std::set<Resource*>									m_DirtyDescriptorSetAccelerationStructures;
	};
}
