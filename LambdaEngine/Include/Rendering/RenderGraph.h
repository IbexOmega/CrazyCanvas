#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"
#include "RenderGraphTypes.h"

#include <unordered_map>
#include <set>

namespace LambdaEngine
{
	struct TextureViewDesc;
	struct TextureDesc;
	struct SamplerDesc;
	struct BufferDesc;

	class ITopLevelAccelerationStructure;
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
		const RenderStageDesc* pRenderStages	= nullptr;
		uint32 RenderStageCount					= 0;
	};

	struct ResourceUpdateDesc
	{
		const char* pName					= "No Resource Name";

		union
		{
			struct
			{
				void*				pData;
				uint32				DataSize;
			} PushConstantUpdate;

			struct
			{
				TextureDesc*		pTextureDesc;
				TextureViewDesc*	pTextureViewDesc;
				SamplerDesc*		pSamplerDesc;
			} InternalTextureUpdate;

			struct
			{
				BufferDesc*			pBufferDesc;
			} InternalBufferUpdate;

			struct
			{
				ITexture*			pTexture;
				ITextureView*		pTextureView;
				ISampler*			pSampler;
			} ExternalTextureUpdate;

			struct
			{
				IBuffer*			pBuffer;
			} ExternalBufferUpdate;

			struct
			{
				ITopLevelAccelerationStructure* pTLAS;
			} ExternalAccelerationStructure;
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
			INTERNAL_COLOR_ATTACHMENT			= 4,
			INTERNAL_DEPTH_STENCIL_ATTACHMENT	= 5,
			EXTERNAL_TEXTURE					= 6,
			EXTERNAL_BUFFER						= 7,
			EXTERNAL_ACCELERATION_STRUCTURE		= 8,
		};

		struct ResourceBinding
		{
			IDescriptorSet*	pDescriptorSet	= nullptr;
			uint32			Binding			= 0;
		};

		struct Resource
		{
			EResourceType Type	= EResourceType::UNKNOWN;
			std::vector<ResourceBinding>		 ResourceBindings;

			union
			{
				struct 
				{
					void*			pData;
					uint32			DataSize;
				} PushConstants;

				struct
				{
					std::vector<PipelineTextureBarrier*> Barriers;
					ITexture*		pTexture;
					ITextureView*	pTextureView;
					ISampler*		pSampler;
				} Texture;

				struct
				{
					//std::vector<PipelineBufferBarrier*> Barriers;
					IBuffer*		pBuffer;
				} Buffer;

				struct
				{
					ITopLevelAccelerationStructure* pTLAS;
				} AccelerationStructure;
			};
		};

		struct RenderStage
		{
			uint64				WaitValue				= 0;
			uint64				SignalValue				= 0;

			EDrawType			DrawType				= EDrawType::NONE;

			IPipelineLayout*	pPipelineLayout			= nullptr;
			IPipelineState*		pPipelineState			= nullptr;
			IDescriptorSet*		pDescriptorSet			= nullptr;

			Resource*			pPushConstantsResource	= nullptr;
			std::set<Resource*> pRenderPassResources;
		};

		struct TextureSynchronization
		{
			FShaderStageFlags		SrcShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage = FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			PipelineTextureBarrier	Barrier;
		};

		struct SynchronizationStage
		{
			std::unordered_map<const char*, TextureSynchronization>		TextureSynchronizations;

			//std::unordered_map<const char*, PipelineBufferBarrier>	BufferBarriers;
		};

		struct PipelineStage
		{
			EPipelineStageType Type		= EPipelineStageType::NONE;
			uint32 StageIndex			= 0;

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

		bool Init(const RenderGraphDesc& desc);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc& desc);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets, the RenderGraph will wait for device idle if it needs to
		*/
		void Update();

		/*
		* 
		*/
		void Render();

	private:
		bool CreateDescriptorHeap();
		bool CreateResources(const std::vector<RenderStageResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions);
		bool CreateSynchronizationStages(const std::vector<SynchronizationStageDesc>& synchronizationStageDescriptions);
		bool CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions);

		void UpdateResourcePushConstants(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalColorAttachment(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceInternalDepthStencilAttachment(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalTexture(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalBuffer(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceExternalAccelerationStructure(const char* pResourceName, Resource* pResource, const ResourceUpdateDesc& desc);

		void ExecuteSynchronizationStage(SynchronizationStage* pSynchronizationStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, ICommandAllocator* pGraphicsCommandAllocator, ICommandList* pGraphicsCommandList);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, ICommandAllocator* pComputeCommandAllocator, ICommandList* pComputeCommandList);

		uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		IDescriptorHeap*									m_pDescriptorHeap;

		PipelineStage*										m_pPipelineStages;
		uint32												m_PipelineStageCount;
		RenderStage*										m_pRenderStages;
		SynchronizationStage*								m_pSynchronizationStages;

		std::unordered_map<const char*, Resource>			m_ResourceMap;
		std::set<Resource*>									m_DirtyDescriptorSetTextures;
		std::set<Resource*>									m_DirtyDescriptorSetBuffers;
		std::set<Resource*>									m_DirtyDescriptorSetAccelerationStructures;
	};
}