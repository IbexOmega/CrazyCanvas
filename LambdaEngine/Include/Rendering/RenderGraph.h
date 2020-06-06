#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/CommandList.h"
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

	class AccelerationStructure;
	class CommandAllocator;
	class GraphicsDevice;
	class PipelineLayout;
	class PipelineState;
	class DescriptorSet;
	class DescriptorHeap;
	class CommandList;
	class TextureView;
	class Texture;
	class Sampler;
	class Buffer;
	class Fence;
	class Scene;

	struct RenderGraphDesc
	{
		const char* pName						= "Render Graph";
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
				Texture**			ppTextures;
				TextureView**		ppTextureViews;
				Sampler**			ppSamplers;
			} ExternalTextureUpdate;

			struct
			{
				Buffer**			ppBuffer;
			} ExternalBufferUpdate;

			struct
			{
				AccelerationStructure* pTLAS;
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
			EDescriptorType DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
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
				std::vector<uint32>			Barriers; //Divided into #SubResourceCount Barriers per Synchronization Stage
				std::vector<Texture*>		Textures;
				std::vector<TextureView*>	TextureViews;
				std::vector<Sampler*>		Samplers;
			} Texture;

			struct
			{
				std::vector<uint32>			Barriers;
				std::vector<Buffer*>		Buffers;
				std::vector<uint64>			Offsets;
				std::vector<uint64>			SizesInBytes;
			} Buffer;

			struct
			{
				AccelerationStructure* pTLAS;
			} AccelerationStructure;
		};

		struct RenderStage
		{
			ERenderStageDrawType	DrawType						= ERenderStageDrawType::NONE;
			Resource*				pVertexBufferResource			= nullptr;
			Resource*				pIndexBufferResource			= nullptr;
			Resource*				pMeshIndexBufferResource		= nullptr;

			RenderStageParameters	Parameters						= {};
			uint64					PipelineStateID					= 0;
			EPipelineStateType		PipelineStateType				= EPipelineStateType::PIPELINE_STATE_TYPE_NONE;
			PipelineLayout*		pPipelineLayout					= nullptr;
			uint32					TextureSubDescriptorSetCount	= 1;
			uint32					MaterialsRenderedPerPass		= 1;
			DescriptorSet**		ppTextureDescriptorSets			= nullptr; //# m_BackBufferCount * ceil(# Textures per Draw / m_MaxTexturesPerDescriptorSet)
			DescriptorSet**		ppBufferDescriptorSets			= nullptr; //# m_BackBufferCount
			RenderPass*			pRenderPass						= nullptr;

			Resource*				pPushConstantsResource			= nullptr;
			std::vector<Resource*>	RenderTargetResources;
			Resource*				pDepthStencilAttachment			= nullptr;
		};

		struct TextureSynchronization
		{
			FShaderStageFlags		SrcShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			uint32					BarrierUseFrameIndex	= 0;
			uint32					SameFrameBarrierOffset	= 1;
			std::vector<uint32>		Barriers;
		};

		struct BufferSynchronization
		{
			FShaderStageFlags		SrcShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			FShaderStageFlags		DstShaderStage			= FShaderStageFlags::SHADER_STAGE_FLAG_NONE;
			uint32					BarrierUseFrameIndex	= 0;
			uint32					SameFrameBarrierOffset	= 1;
			std::vector<uint32>		Barriers;
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

			CommandAllocator** ppGraphicsCommandAllocators;
			CommandAllocator** ppComputeCommandAllocators;
			CommandList** ppGraphicsCommandLists;
			CommandList** ppComputeCommandLists;
		};

	public:
		DECL_REMOVE_COPY(RenderGraph);
		DECL_REMOVE_MOVE(RenderGraph);

		RenderGraph(const GraphicsDevice* pGraphicsDevice);
		~RenderGraph();

		bool Init(RenderGraphDesc& desc);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc& desc);

		void UpdateRenderStageParameters(const RenderStageParameters& desc);

		void GetAndIncrementFence(Fence** ppFence, uint64* pSignalValue);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets, the RenderGraph will wait for device idle if it needs to
		*/
		void Update();
		void Render(uint64 frameIndex, uint32 backBufferIndex);

		bool GetResourceTextures(const char* pResourceName, Texture* const ** pppTexture, uint32* pTextureView)					const;
		bool GetResourceTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount)		const;
		bool GetResourceBuffers(const char* pResourceName, Buffer* const ** pppBuffers, uint32* pBufferCount)					const;
		bool GetResourceAccelerationStructure(const char* pResourceName, AccelerationStructure** ppAccelerationStructure)		const;

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
			CommandAllocator* pGraphicsCommandAllocator, 
			CommandList* pGraphicsCommandList, 
			CommandAllocator* pComputeCommandAllocator, 
			CommandList* pComputeCommandList, 
			CommandList** ppFirstExecutionStage, 
			CommandList** ppSecondExecutionStage, 
			uint32 backBufferIndex);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pGraphicsCommandAllocator, CommandList* pGraphicsCommandList, CommandList** ppExecutionStage, uint32 backBufferIndex);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage, uint32 backBufferIndex);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage, uint32 backBufferIndex);

		uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc);

	private:
		const GraphicsDevice*								m_pGraphicsDevice;

		const Scene*										m_pScene						= nullptr;

		DescriptorHeap*									m_pDescriptorHeap				= nullptr;

		uint32												m_BackBufferCount				= 0;
		uint32												m_MaxTexturesPerDescriptorSet	= 0;
		
		Fence*												m_pFence						= nullptr;
		uint64												m_SignalValue					= 1;

		CommandList**										m_ppExecutionStages				= nullptr;
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

		std::vector<PipelineTextureBarrierDesc>				m_TextureBarriers;
		std::vector<PipelineBufferBarrierDesc>				m_BufferBarriers;
	};
}
