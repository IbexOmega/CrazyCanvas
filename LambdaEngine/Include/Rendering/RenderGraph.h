#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Buffer.h"

#include "Containers/THashTable.h"
#include "Containers/TArray.h"
#include "Containers/TSet.h"
#include "Containers/String.h"

#include "RenderGraphTypes.h"
#include "RenderGraphEditor.h"

#include "Utilities/StringHash.h"

#include "Time/API/Timestamp.h"

namespace LambdaEngine
{
	struct PipelineTextureBarrierDesc;
	struct PipelineBufferBarrierDesc;

	class AccelerationStructure;
	class CommandAllocator;
	class GraphicsDevice;
	class PipelineLayout;
	class PipelineState;
	class DescriptorSet;
	class DescriptorHeap;
	class ICustomRenderer;
	class CommandList;
	class TextureView;
	class Texture;
	class Sampler;
	class Buffer;
	class Fence;
	class Scene;

	struct RenderGraphDesc
	{
		String Name											= "Render Graph";
		RenderGraphStructureDesc* pRenderGraphStructureDesc	= nullptr;
		uint32 BackBufferCount								= 3;
		uint32 MaxTexturesPerDescriptorSet					= 1;
		const Scene* pScene									= nullptr;
	};

	struct ResourceUpdateDesc
	{
		String ResourceName	= "No Resource Name";

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
				const AccelerationStructure* pTLAS;
			} ExternalAccelerationStructure;
		};
	};

	struct MaterialBindingInfo
	{
		uint32	Stride;
	};

	class LAMBDA_API RenderGraph : public EventHandler
	{
		enum class EResourceOwnershipType
		{
			NONE				= 0,
			INTERNAL			= 1,
			EXTERNAL			= 2,
		};

		struct RenderStage;

		struct ResourceBinding
		{
			RenderStage*	pRenderStage	= nullptr;
			EDescriptorType DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
			uint32			Binding			= 0;

			ETextureState	TextureState		= ETextureState::TEXTURE_STATE_UNKNOWN;
		};

		struct ResourceBarrierInfo
		{
			uint32	SynchronizationStageIndex	= 0;
			uint32	SynchronizationTypeIndex	= 0;
			uint32	BarrierIndex				= 0;
		};

		struct InternalResourceUpdateDesc
		{
			String						ResourceName	= "No Resource Name";
			ERenderGraphResourceType	Type			= ERenderGraphResourceType::NONE;

			struct
			{
				ERenderGraphDimensionType	XDimType;
				ERenderGraphDimensionType	YDimType;
				float32						XDimVariable;
				float32						YDimVariable;

				TextureDesc					TextureDesc;
				TextureViewDesc				TextureViewDesc;
				SamplerDesc					SamplerDesc;
			} TextureUpdate;

			struct
			{
				ERenderGraphDimensionType	SizeType;

				BufferDesc		BufferDesc;
			} BufferUpdate;
		};

		struct Resource
		{
			String						Name				= "";
			bool						IsBackBuffer		= false;
			ERenderGraphResourceType	Type				= ERenderGraphResourceType::NONE;
			EResourceOwnershipType		OwnershipType		= EResourceOwnershipType::NONE;
			bool						BackBufferBound		= false;
			uint32						SubResourceCount	= 0;

			TArray<ResourceBinding>	ResourceBindings;

			struct
			{
				bool								IsOfArrayType	= false;
				EFormat								Format			= EFormat::FORMAT_NONE;
				TArray<ResourceBarrierInfo>			BarriersPerSynchronizationStage; //Divided into #SubResourceCount Barriers per Synchronization Stage
				TArray<Texture*>					Textures;
				TArray<TextureView*>				TextureViews;
				TArray<Sampler*>					Samplers;
			} Texture;

			struct
			{
				TArray<ResourceBarrierInfo>			BarriersPerSynchronizationStage;
				TArray<Buffer*>					Buffers;
				TArray<uint64>						Offsets;
				TArray<uint64>						SizesInBytes;
			} Buffer;

			struct
			{
				const AccelerationStructure* pTLAS;
			} AccelerationStructure;
		};

		struct RenderStage
		{
			String					Name							= "";
			RenderStageParameters	Parameters						= {};

			glm::uvec3				Dimensions						= glm::uvec3(0);

			bool					UsesCustomRenderer				= false;
			ICustomRenderer*		pCustomRenderer					= nullptr;

			FPipelineStageFlags		FirstPipelineStage				= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags		LastPipelineStage				= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;

			ERenderStageDrawType	DrawType						= ERenderStageDrawType::NONE;
			Resource*				pIndexBufferResource			= nullptr;
			Resource*				pIndirectArgsBufferResource		= nullptr;

			uint64					PipelineStateID					= 0;
			PipelineLayout*		pPipelineLayout					= nullptr;
			uint32					TextureSubDescriptorSetCount	= 1;
			uint32					MaterialsRenderedPerPass		= 1;
			DescriptorSet**		ppTextureDescriptorSets			= nullptr; //# m_BackBufferCount * ceil(# Textures per Draw / m_MaxTexturesPerDescriptorSet)
			DescriptorSet**		ppBufferDescriptorSets			= nullptr; //# m_BackBufferCount
			RenderPass*			pRenderPass						= nullptr;

			Resource*				pPushConstantsResource			= nullptr;
			TArray<Resource*>		RenderTargetResources;
			Resource*				pDepthStencilAttachment			= nullptr;
		};

		struct SynchronizationStage
		{
			ECommandQueueType					ExecutionQueue					= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			FPipelineStageFlags					SrcPipelineStage				= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags					SameQueueDstPipelineStage		= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags					OtherQueueDstPipelineStage		= FPipelineStageFlags::PIPELINE_STAGE_FLAG_UNKNOWN;

			TArray<PipelineBufferBarrierDesc>	BufferBarriers[2];
			TArray<PipelineTextureBarrierDesc>	TextureBarriers[4];
		};

		struct PipelineStage
		{
			ERenderGraphPipelineStageType	Type							= ERenderGraphPipelineStageType::NONE;
			uint32							StageIndex						= 0;

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

		bool Init(const RenderGraphDesc* pDesc);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc& desc);
		void UpdateRenderStageDimensions(const String& renderStageName, uint32 x, uint32 y, uint32 z = 0);
		void UpdateResourceDimensions(const String& resourceName, uint32 x, uint32 y = 0);

		void GetAndIncrementFence(Fence** ppFence, uint64* pSignalValue);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets, the RenderGraph will wait for device idle if it needs to
		*/
		void Update();

		void NewFrame(uint64 modFrameIndex, uint32 backBufferIndex, Timestamp delta);
		void PrepareRender(Timestamp delta);

		void Render();

		CommandList* AcquireGraphicsCopyCommandList();
		CommandList* AcquireComputeCopyCommandList();

		bool GetResourceTextures(const char* pResourceName, Texture* const ** pppTexture, uint32* pTextureView)						const;
		bool GetResourceTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount)		const;
		bool GetResourceBuffers(const char* pResourceName, Buffer* const ** pppBuffers, uint32* pBufferCount)							const;
		bool GetResourceAccelerationStructure(const char* pResourceName, const AccelerationStructure** ppAccelerationStructure)		const;

		virtual void OnWindowResized(Window* pWindow, uint16 width, uint16 height, EResizeType type) override;

	private:
		bool CreateFence();
		bool CreateDescriptorHeap();
		bool CreateCopyCommandLists();
		bool CreateResources(const TArray<RenderGraphResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const TArray<RenderStageDesc>& renderStages);
		bool CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions);
		bool CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions);

		void UpdateInternalResource(InternalResourceUpdateDesc& desc);

		void UpdateResourceTexture(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceBuffer(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateResourceAccelerationStructure(Resource* pResource, const ResourceUpdateDesc& desc);
		void UpdateRelativeRenderStageDimensions(RenderStage* pRenderStage);
		void UpdateRelativeResourceDimensions(InternalResourceUpdateDesc* pResourceUpdateDesc);

		void ExecuteSynchronizationStage(
			SynchronizationStage* pSynchronizationStage, 
			CommandAllocator* pGraphicsCommandAllocator, 
			CommandList* pGraphicsCommandList, 
			CommandAllocator* pComputeCommandAllocator, 
			CommandList* pComputeCommandList, 
			CommandList** ppFirstExecutionStage, 
			CommandList** ppSecondExecutionStage);
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pGraphicsCommandAllocator, CommandList* pGraphicsCommandList, CommandList** ppExecutionStage);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, PipelineState* pPipelineState, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage);

	private:
		const GraphicsDevice*				m_pGraphicsDevice;

		const Scene*									m_pScene							= nullptr;

		DescriptorHeap*					m_pDescriptorHeap					= nullptr;

		float32											m_WindowWidth						= 0.0f;
		float32											m_WindowHeight						= 0.0f;

		uint64											m_FrameIndex						= 0;
		uint64											m_ModFrameIndex						= 0;
		uint32											m_BackBufferIndex					= 0;
		uint32											m_BackBufferCount					= 0;
		uint32											m_MaxTexturesPerDescriptorSet		= 0;

		Fence*								m_pFence							= nullptr;
		uint64								m_SignalValue						= 1;

		TArray<ICustomRenderer*>						m_CustomRenderers;
		TArray<ICustomRenderer*>						m_DebugRenderers;

		CommandAllocator**					m_ppGraphicsCopyCommandAllocators	= nullptr;
		CommandList**						m_ppGraphicsCopyCommandLists		= nullptr;
		bool								m_ExecuteGraphicsCopy				= false;

		CommandAllocator**					m_ppComputeCopyCommandAllocators	= nullptr;
		CommandList**						m_ppComputeCopyCommandLists			= nullptr;
		bool								m_ExecuteComputeCopy				= false;

		CommandList**						m_ppExecutionStages					= nullptr;
		uint32								m_ExecutionStageCount				= 0;

		PipelineStage*									m_pPipelineStages					= nullptr;
		uint32											m_PipelineStageCount				= 0;

		THashTable<String, uint32>						m_RenderStageMap;
		RenderStage*									m_pRenderStages						= nullptr;
		uint32											m_RenderStageCount					= 0;
		TSet<uint32>									m_WindowRelativeRenderStages;		// Contains Render Stage Indices that have Dimension Variables that depend on the current Window Size

		SynchronizationStage*							m_pSynchronizationStages			= nullptr;
		uint32											m_SynchronizationStageCount			= 0;

		THashTable<String, Resource>					m_ResourceMap;
		THashTable<String, InternalResourceUpdateDesc>	m_InternalResourceUpdateDescriptions;
		TArray<String>									m_WindowRelativeResources;
		TSet<String>									m_DirtyInternalResources;

		TSet<Resource*>									m_DirtyDescriptorSetTextures;
		TSet<Resource*>									m_DirtyDescriptorSetBuffers;
		TSet<Resource*>									m_DirtyDescriptorSetAccelerationStructures;
	};
}
