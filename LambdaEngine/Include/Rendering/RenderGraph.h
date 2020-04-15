#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"
#include "RenderGraphTypes.h"

#include <unordered_map>
#include <set>

namespace LambdaEngine
{
	struct PipelineTextureBarrier;
	struct PipelineBufferBarrier;
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

	struct RenderGraphDesc
	{
		const char* pName						= "Render Graph";
		bool CreateDebugGraph					= false;
		RenderStageDesc* pRenderStages			= nullptr;
		uint32 RenderStageCount					= 0;
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
				ITexture**			ppTexture;
				ITextureView**		ppTextureView;
				ISampler**			ppSampler;
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

		struct ResourceBinding
		{
			IDescriptorSet*	pDescriptorSet	= nullptr;
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
				std::vector<PipelineTextureBarrier*> Barriers; //Divided into #SubResourceCount Barriers per Synchronization Stage
				std::vector<ITexture*>		Textures;
				std::vector<ITextureView*>	TextureViews;
				std::vector<ISampler*>		Samplers;
			} Texture;

			struct
			{
				std::vector<PipelineBufferBarrier*> Barriers;
				std::vector<IBuffer*>		Buffers;
				std::vector<uint32>			Offsets;
				std::vector<uint32>			SizesInBytes;
			} Buffer;

			struct
			{
				IAccelerationStructure* pTLAS;
			} AccelerationStructure;
		};

		struct RenderStage
		{
			ERenderStageDrawType	DrawType				= ERenderStageDrawType::NONE;
			Resource*				pDrawResource			= nullptr;

			RenderStageParameters	Parameters				= {};
			IPipelineLayout*		pPipelineLayout			= nullptr;
			IPipelineState*			pPipelineState			= nullptr;
			IDescriptorSet*			pDescriptorSet			= nullptr;
			IRenderPass*			pRenderPass				= nullptr;

			Resource*				pPushConstantsResource	= nullptr;
			std::set<Resource*>		pRenderPassResources;
		};

		struct TextureSynchronization
		{
			FShaderStageFlags		SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			std::vector<PipelineTextureBarrier> Barriers;
		};

		struct BufferSynchronization
		{
			FShaderStageFlags		SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			std::vector<PipelineBufferBarrier> Barriers;
		};

		struct SynchronizationStage
		{
			std::unordered_map<const char*, TextureSynchronization> TextureSynchronizations;
			std::unordered_map<const char*, BufferSynchronization> BufferSynchronizations;
		};

		struct PipelineStage
		{
			EPipelineStageType	Type			= EPipelineStageType::NONE;
			uint32				StageIndex		= 0;

			ICommandAllocator* pGraphicsCommandAllocators[MAX_FRAMES_IN_FLIGHT];
			ICommandAllocator* pComputeCommandAllocators[MAX_FRAMES_IN_FLIGHT];
			ICommandList* pGraphicsCommandLists[MAX_FRAMES_IN_FLIGHT];
			ICommandList* pComputeCommandLists[MAX_FRAMES_IN_FLIGHT];
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
		void Render(uint32 frameIndex);

	private:
		bool CreateDescriptorHeap();
		bool CreateResources(const std::vector<RenderStageResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions);
		bool CreateSynchronizationStages(const std::vector<SynchronizationStageDesc>& synchronizationStageDescriptions);
		bool CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions);

		void UpdateResourcePushConstants(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalAccelerationStructure(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);

		void ExecuteSynchronizationStage(
			SynchronizationStage* pSynchronizationStage, 
			ICommandAllocator* pGraphicsCommandAllocator, 
			ICommandList* pGraphicsCommandList, 
			ICommandAllocator* pComputeCommandAllocator, 
			ICommandList* pComputeCommandList, 
			ICommandList** ppFirstExecutionStage, 
			ICommandList** ppSecondExecutionStage);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList, ICommandList** ppExecutionStage);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList, ICommandList** ppExecutionStage);

		uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		IDescriptorHeap*									m_pDescriptorHeap;

		ICommandList**										m_ppExecutionStages;
		uint32												m_ExecutionStageCount;
		PipelineStage*										m_pPipelineStages;
		uint32												m_PipelineStageCount;
		std::unordered_map<const char*, uint32>				m_RenderStageMap;
		RenderStage*										m_pRenderStages;
		SynchronizationStage*								m_pSynchronizationStages;

		std::unordered_map<const char*, Resource>			m_ResourceMap;
		std::set<Resource*>									m_DirtyDescriptorSetTextures;
		std::set<Resource*>									m_DirtyDescriptorSetBuffers;
		std::set<Resource*>									m_DirtyDescriptorSetAccelerationStructures;
	};
}