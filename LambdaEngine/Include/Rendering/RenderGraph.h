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

#include "Application/API/Events/WindowEvents.h"
#include "Application/API/Events/DebugEvents.h"
#include "Application/API/Events/RenderEvents.h"

namespace LambdaEngine
{
	struct PipelineTextureBarrierDesc;
	struct PipelineBufferBarrierDesc;

	class IRenderGraphCreateHandler;
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
	class SBT;

	struct RenderGraphDesc
	{
		String Name											= "Render Graph";
		RenderGraphStructureDesc* pRenderGraphStructureDesc	= nullptr;
		uint32 BackBufferCount								= 3;
		TArray<ICustomRenderer*>	CustomRenderers;
	};

	struct PushConstantsUpdate
	{
		String	RenderStageName = "";
		void*	pData			= nullptr;
		uint32	DataSize		= 0;
	};

	struct ResourceUpdateDesc
	{
		String ResourceName	= "No Resource Name";

		union
		{
			struct
			{
				TextureDesc*		pTextureDesc;
				TextureViewDesc*	pTextureViewDesc;
				SamplerDesc*		pSamplerDesc;
			} InternalTextureUpdate;

			struct
			{
				BufferDesc*	pBufferDesc;
			} InternalBufferUpdate;

			struct
			{
				Texture**		ppTextures;
				TextureView**	ppTextureViews;
				TextureView**	ppPerSubImageTextureViews;
				Sampler**		ppSamplers;
				uint32			Count;
				uint32			PerImageSubImageTextureViewCount;
			} ExternalTextureUpdate;

			struct
			{
				uint32		DrawArgsMask;
				DrawArg*	pDrawArgs;
				uint32		Count;
			} ExternalDrawArgsUpdate;

			struct
			{
				Buffer**	ppBuffer;
				uint32		Count;
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

	class LAMBDA_API RenderGraph
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
			RenderStage*	pRenderStage		= nullptr;
			EDescriptorType DescriptorType		= EDescriptorType::DESCRIPTOR_TYPE_UNKNOWN;
			uint32			Binding				= 0;

			ETextureState	TextureState		= ETextureState::TEXTURE_STATE_UNKNOWN;
		};

		struct ResourceBarrierInfo
		{
			uint32	SynchronizationStageIndex	= 0;
			uint32	SynchronizationTypeIndex	= 0;
			uint32	BarrierIndex				= 0;
			uint32	DrawArgsMask				= 0x0;
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

		struct PushConstants
		{
			byte*	pData		= nullptr;
			uint32	DataSize	= 0;
			uint32	Offset		= 0;
			uint32	MaxDataSize	= 0;
		};

		struct DrawArgsData
		{
			PipelineBufferBarrierDesc			InitialTransitionBarrierTemplate;
			PipelineTextureBarrierDesc			InitialTextureTransitionBarrierTemplate;
			TArray<DrawArg>						Args;
		};

		struct Resource
		{
			String						Name				= "";
			bool						IsBackBuffer		= false;
			ERenderGraphResourceType	Type				= ERenderGraphResourceType::NONE;
			EResourceOwnershipType		OwnershipType		= EResourceOwnershipType::NONE;
			bool						BackBufferBound		= false;
			uint32						SubResourceCount	= 0;
			bool						ShouldSynchronize	= true;

			TArray<ResourceBinding>		ResourceBindings;
			TArray<ResourceBarrierInfo>	BarriersPerSynchronizationStage; //Divided into #SubResourceCount Barriers per Synchronization Stage

			FPipelineStageFlags			LastPipelineStageOfFirstRenderStage = FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;

			struct
			{
				ERenderGraphTextureType				TextureType						= ERenderGraphTextureType::TEXTURE_2D;
				bool								IsOfArrayType					= false;
				bool								UnboundedArray					= false;
				bool								UsedAsRenderTarget				= false;
				bool								PerSubImageUniquelyAllocated	= false;
				EFormat								Format							= EFormat::FORMAT_NONE;
				PipelineTextureBarrierDesc			InitialTransitionBarrier;
				TArray<Texture*>					Textures;
				TArray<TextureView*>				PerImageTextureViews;
				TArray<TextureView*>				PerSubImageTextureViews;
				TArray<Sampler*>					Samplers;
			} Texture;

			struct
			{
				THashTable<uint32, DrawArgsData> MaskToArgs;
			} DrawArgs;

			struct
			{
				PipelineBufferBarrierDesc			InitialTransitionBarrier;
				TArray<Buffer*>						Buffers;
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
			String					Name								= "";
			RenderStageParameters	Parameters							= {};

			/*
				If set to true, renderstage will not be submited during execution of rendergraph.
				*Triggertype will be ignored*
				Use RenderSystem->setRenderStageSleeping to enable/disable sleeping outside of rendergraph
			*/
			bool							Sleeping			= false;

			//Triggering
			ERenderStageExecutionTrigger	TriggerType					= ERenderStageExecutionTrigger::NONE;
			uint32							FrameDelay					= 0;
			uint32							FrameOffset					= 0;
			uint32							FrameCounter				= 0;

			//Special Draw Params
			uint32					ExecutionCount						= 1;

			glm::uvec3				Dimensions							= glm::uvec3(0);

			bool					UsesCustomRenderer					= false;
			ICustomRenderer*		pCustomRenderer						= nullptr;

			FPipelineStageFlags		FirstPipelineStage					= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags		LastPipelineStage					= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
			uint32					PipelineStageMask					= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;

			ERenderStageDrawType	DrawType							= ERenderStageDrawType::NONE;

			uint64					PipelineStateID						= 0;
			PipelineState*			pPipelineState						= nullptr;
			PipelineLayout*			pPipelineLayout						= nullptr;
			SBT*					pSBT								= nullptr;
			DescriptorSet**			ppBufferDescriptorSets				= nullptr; //# m_BackBufferCount
			uint32					BufferSetIndex						= 0;
			DescriptorSet**			ppTextureDescriptorSets				= nullptr; //# m_BackBufferCount
			uint32					TextureSetIndex						= 0;
			DescriptorSet***		pppDrawArgDescriptorSets			= nullptr; //# m_BackBufferCount
			DescriptorSet***		pppDrawArgExtensionsDescriptorSets	= nullptr; //# m_BackBufferCount
			DrawArg*				pDrawArgs							= nullptr;
			uint32					NumDrawArgsPerFrame					= 0;
			uint32					DrawSetIndex						= 0;
			uint32					DrawExtensionSetIndex				= 0;
			Resource*				pDrawArgsResource					= nullptr;
			uint32					DrawArgsMask						= 0x0;
			RenderPass*				pRenderPass							= nullptr;
			RenderPass*				pDisabledRenderPass					= nullptr;

			PushConstants			pInternalPushConstants[NUM_INTERNAL_PUSH_CONSTANTS_TYPES];
			PushConstants			ExternalPushConstants		= {};
			TArray<Resource*>		RenderTargetResources;
			Resource*				pDepthStencilAttachment		= nullptr;
		};

		struct SynchronizationStage
		{
			ECommandQueueType		ExecutionQueue				= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
			FPipelineStageFlags		SrcPipelineStage			= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags		SameQueueDstPipelineStage	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
			FPipelineStageFlags		OtherQueueDstPipelineStage	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN;
			uint32					DrawArgsMask				= 0x0;

			TArray<PipelineTextureBarrierDesc>			DrawTextureBarriers[4];
			TArray<PipelineBufferBarrierDesc>			DrawBufferBarriers[2]; //This works while we only have one Draw Arg resource, otherwise we have to do like Unbounded Textures
			TArray<PipelineBufferBarrierDesc>			BufferBarriers[2];
			TArray<PipelineTextureBarrierDesc>			TextureBarriers[4];
			TArray<TArray<PipelineTextureBarrierDesc>>	UnboundedTextureBarriers[2]; //Unbounded Arrays of textures do not have a known count at init time -> we cant store them densely
		};

		struct PipelineStage
		{
			ERenderGraphPipelineStageType	Type							= ERenderGraphPipelineStageType::NONE;
			uint32							StageIndex						= 0;

			CommandAllocator** ppGraphicsCommandAllocators	= nullptr;
			CommandAllocator** ppComputeCommandAllocators	= nullptr;
			CommandList** ppGraphicsCommandLists			= nullptr;
			CommandList** ppComputeCommandLists				= nullptr;
		};

	public:
		DECL_REMOVE_COPY(RenderGraph);
		DECL_REMOVE_MOVE(RenderGraph);

		RenderGraph(const GraphicsDevice* pGraphicsDevice);
		~RenderGraph();

		bool Init(const RenderGraphDesc* pDesc, TSet<uint32>& requiredDrawArgs);

		/*
		* Recreates the Render Graph according to pDesc.
		* Discoveres overlapping resources from the current Render Graph and the new Render Graph and reuses them.
		*/
		bool Recreate(const RenderGraphDesc* pDesc, TSet<uint32>& requiredDrawArgs);

		/*
		* Adds a Create Handler to this Render Graph, all IRenderGraphCreateHandler::OnRenderGraphRecreate
		* gets called when RenderGraph::Recreate is called.
		* This can get used to initialize new resources in the Render Graph.
		*/
		void AddCreateHandler(IRenderGraphCreateHandler* pCreateHandler);

		/*
		* Updates a resource in the Render Graph, can be called at any time
		*	desc - The ResourceUpdateDesc, only the Update Parameters for the given update type should be set
		*/
		void UpdateResource(const ResourceUpdateDesc* pDesc);
		/*
		* Updates Push Constants for a given Render Stage, PushConstantsUpdate::DataSize needs to be less than or equal to the
		* RenderStage::ExternalPushConstants.MaxDataSize which is set depending on the Bindings and type of Render Stage
		*/
		void UpdatePushConstants(const PushConstantsUpdate* pDesc);
		/*
		* Updates the global SBT which is used for all Ray Tracing calls, each SBTRecord should contain addresses to valid Buffers
		*/
		void UpdateGlobalSBT(const TArray<SBTRecord>& shaderRecords);
		/*
		* Updates the dimensions of a RenderStage, will only set the dimensions which are set to EXTERNAL
		*/
		void UpdateRenderStageDimensions(const String& renderStageName, uint32 x, uint32 y, uint32 z = 0);
		/*
		* Updates the dimensions of a resource, will only set the dimensions which are set to EXTERNAL
		*/
		void UpdateResourceDimensions(const String& resourceName, uint32 x, uint32 y = 0);

		/*
		* Triggers a Render Stage which has a TriggerType of Triggered
		*/
		void TriggerRenderStage(const String& renderStageName);

		/*
		* Puts given render stage to sleep, this will prevent execution of renderstage
		* Useful for optimization when a rendergraph can still execute without given renderstage
		*/
		void SetRenderStageSleeping(const String& renderStageName, bool sleeping);

		/*
		* Updates the RenderGraph, applying the updates made to resources with UpdateResource by writing them to the appropriate Descriptor Sets
		*/
		void Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex);

		/*
		* Updates dirty resource bindings
		*/
		void UpdateResourceBindings();


		/*
		* Executes the RenderGraph, goes through each Render Stage and Synchronization Stage and executes them.
		*	If the RenderGraph writes to the Back Buffer it is safe to present the Back Buffer after Render has returned.
		*/
		void Render(uint64 modFrameIndex, uint32 backBufferIndex);

		/*
		* Acquires a general purpose Graphics Command List, this will then be executed before all other Render Stages & Synchronization Stages.
		*	-----NOT THREADSAFE-----
		*/
		CommandList* AcquireGraphicsCopyCommandList();
		/*
		* Acquires a general purpose Compute Command List, this will then be executed before all other Render Stages & Synchronization Stages.
		*	-----NOT THREADSAFE-----
		*/
		CommandList* AcquireComputeCopyCommandList();

		bool GetResourceTextures(const char* pResourceName, Texture* const ** pppTexture, uint32* pTextureView)									const;
		bool GetResourcePerImageTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount)		const;
		bool GetResourcePerSubImageTextureViews(const char* pResourceName, TextureView* const ** pppTextureViews, uint32* pTextureViewCount)	const;
		bool GetResourceBuffers(const char* pResourceName, Buffer* const ** pppBuffers, uint32* pBufferCount)									const;
		bool GetResourceAccelerationStructure(const char* pResourceName, const AccelerationStructure** ppAccelerationStructure)					const;

	private:
		bool OnWindowResized(const WindowResizedEvent& windowEvent);
		bool OnPreSwapChainRecreated(const PreSwapChainRecreatedEvent& swapChainEvent);
		bool OnPostSwapChainRecreated(const PostSwapChainRecreatedEvent& swapChainEvent);
		bool OnPipelineStatesRecompiled(const PipelineStatesRecompiledEvent& event);

		void ReleasePipelineStages();

		bool CreateFence();
		bool CreateDescriptorHeap();
		bool CreateCopyCommandLists();
		bool CreateProfiler(uint32 pipelineStageCount);
		bool CreateResources(const TArray<RenderGraphResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const TArray<RenderStageDesc>& renderStages, const THashTable<String, RenderGraphShaderConstants>& shaderConstants, const TArray<ICustomRenderer*>& customRenderers, TSet<uint32>& requiredDrawArgs);
		bool CreateSynchronizationStages(const TArray<SynchronizationStageDesc>& synchronizationStageDescriptions, TSet<uint32>& requiredDrawArgs);
		bool CreatePipelineStages(const TArray<PipelineStageDesc>& pipelineStageDescriptions);

		void UpdateRelativeParameters();
		void UpdateInternalResource(InternalResourceUpdateDesc& desc);

		void UpdateResourceTexture(Resource* pResource, const ResourceUpdateDesc* pDesc);
		void UpdateResourceDrawArgs(Resource* pResource, const ResourceUpdateDesc* pDesc);
		void UpdateResourceBuffer(Resource* pResource, const ResourceUpdateDesc* pDesc);
		void UpdateResourceAccelerationStructure(Resource* pResource, const ResourceUpdateDesc* pDesc);
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
		void ExecuteGraphicsRenderStage(RenderStage* pRenderStage, CommandAllocator* pGraphicsCommandAllocator, CommandList* pGraphicsCommandList, CommandList** ppExecutionStage);
		void ExecuteComputeRenderStage(RenderStage* pRenderStage, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage);
		void ExecuteRayTracingRenderStage(RenderStage* pRenderStage, CommandAllocator* pComputeCommandAllocator, CommandList* pComputeCommandList, CommandList** ppExecutionStage);

		// Helpers
		void PipelineTextureBarriers(CommandList* pCommandList, const TArray<PipelineTextureBarrierDesc>& textureBarriers, FPipelineStageFlags srcPipelineStage, FPipelineStageFlags dstPipelineStage);
		void PipelineBufferBarriers(CommandList* pCommandList, const TArray<PipelineBufferBarrierDesc>& textureBarriers, FPipelineStageFlags srcPipelineStage, FPipelineStageFlags dstPipelineStage);

	private:
		const GraphicsDevice*							m_pGraphicsDevice;
		GraphicsDeviceFeatureDesc						m_Features;

		DescriptorHeap*									m_pDescriptorHeap					= nullptr;

		float32											m_WindowWidth						= 0.0f;
		float32											m_WindowHeight						= 0.0f;

		uint64											m_FrameIndex						= 0;
		uint64											m_ModFrameIndex						= 0;
		uint32											m_BackBufferIndex					= 0;
		uint32											m_BackBufferCount					= 0;

		Fence*											s_pMaterialFence					= nullptr;
		uint64											m_SignalValue						= 1;

		TArray<ICustomRenderer*>						m_CustomRenderers;
		TArray<ICustomRenderer*>						m_DebugRenderers;

		CommandAllocator**								m_ppGraphicsCopyCommandAllocators	= nullptr;
		CommandList**									m_ppGraphicsCopyCommandLists		= nullptr;

		CommandAllocator**								m_ppComputeCopyCommandAllocators	= nullptr;
		CommandList**									m_ppComputeCopyCommandLists			= nullptr;

		CommandList**									m_ppExecutionStages					= nullptr;
		uint32											m_ExecutionStageCount				= 0;

		PipelineStage*									m_pPipelineStages					= nullptr;
		uint32											m_PipelineStageCount				= 0;

		THashTable<String, uint32>						m_RenderStageMap;
		RenderStage*									m_pRenderStages						= nullptr;
		uint32											m_RenderStageCount					= 0;
		TSet<uint32>									m_WindowRelativeRenderStages;		// Contains Render Stage Indices that have Dimension Variables that depend on the current Window Size
		TArray<SBTRecord>								m_GlobalShaderRecords;

		SynchronizationStage*							m_pSynchronizationStages			= nullptr;
		uint32											m_SynchronizationStageCount			= 0;

		THashTable<String, Resource>					m_ResourceMap;
		THashTable<String, InternalResourceUpdateDesc>	m_InternalResourceUpdateDescriptions;
		TArray<String>									m_WindowRelativeResources;
		TSet<String>									m_DirtyInternalResources;

		TSet<Resource*>									m_DirtyBoundTextureResources;
		TSet<Resource*>									m_DirtyBoundBufferResources;
		TSet<Resource*>									m_DirtyBoundAccelerationStructureResources;
		TSet<Resource*>									m_DirtyBoundDrawArgResources;

		TSet<RenderStage*>								m_DirtyRenderStageTextureSets;
		TSet<RenderStage*>								m_DirtyRenderStageBufferSets;

		TArray<DeviceChild*>*							m_pDeviceResourcesToDestroy;

		TArray<IRenderGraphCreateHandler*>				m_CreateHandlers;
	};
}
