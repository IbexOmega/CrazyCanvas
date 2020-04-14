#pragma once

#include "LambdaEngine.h"

#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/GraphicsTypes.h"
#include "RenderGraphTypes.h"

#include <unordered_map>

namespace LambdaEngine
{
	struct TextureDesc;
	struct SamplerDesc;
	struct BufferDesc;

	class ICommandAllocator;
	class IGraphicsDevice;
	class IPipelineLayout;
	class IPipelineState;
	class IDescriptorSet;
	class IDescriptorHeap;
	class ICommandList;
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
				void* pData;
				uint32 DataSize;
			} PushConstantUpdate;

			struct
			{
				TextureDesc* pTextureDesc;
				SamplerDesc* pSamplerDesc;
				void* pTextureData;
			} InternalTextureUpdate;

			struct
			{
				BufferDesc* pBufferDesc;
				void* pBufferData;
			} InternalBufferUpdate;

			struct
			{
				ITexture* pTexture;
				ISampler* pSampler;
			} ExternalTextureUpdate;

			struct
			{
				IBuffer* pBuffer;
			} ExternalBufferUpdate;
		};
	};

	class LAMBDA_API RenderGraph
	{
		struct RenderStage
		{
			uint64				WaitValue			= 0;
			uint64				SignalValue			= 0;

			IPipelineLayout*	pPipelineLayout		= nullptr;
			IPipelineState*		pPipelineState		= nullptr;
			IDescriptorSet*		pDescriptorSet		= nullptr;
		};

		struct SynchronizationStage
		{
			std::unordered_map<const char*, PipelineTextureBarrier>		TextureBarriers;

			//std::unordered_map<const char*, PipelineBufferBarrier>	BufferBarriers;
		};

		struct PipelineStage
		{
			EPipelineStageType Type = EPipelineStageType::NONE;
			uint32 StageIndex = 0;

			ICommandAllocator* pGraphicsCommandAllocators[MAX_FRAMES_IN_FLIGHT];
			ICommandAllocator* pComputeCommandAllocators[MAX_FRAMES_IN_FLIGHT];
			ICommandList* pGraphicsCommandLists[MAX_FRAMES_IN_FLIGHT];
			ICommandList* pComputeCommandLists[MAX_FRAMES_IN_FLIGHT];
		};

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
			RenderStage*	pRenderStage	= nullptr;
			uint32			Binding			= 0;
		};

		struct Resource
		{
			EResourceType Type	= EResourceType::UNKNOWN;
			std::vector<ResourceBinding> ResourceBindings;

			union
			{
				struct
				{
					ITexture* pTexture;
					ISampler* pSampler;
				} Texture;

				struct
				{
					IBuffer* pBuffer;
				} Buffer;
			};
		};

	public:
		DECL_REMOVE_COPY(RenderGraph);
		DECL_REMOVE_MOVE(RenderGraph);

		RenderGraph(const IGraphicsDevice* pGraphicsDevice);
		~RenderGraph();

		bool Init(const RenderGraphDesc& desc);

		void UpdateResource(const ResourceUpdateDesc& desc);

	private:
		bool CreateDescriptorHeap();
		bool CreateResources(const std::vector<RenderStageResourceDesc>& resourceDescriptions);
		bool CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions);
		bool CreateSynchronizationStages(const std::vector<SynchronizationStageDesc>& synchronizationStageDescriptions);
		bool CreatePipelineStages(const std::vector<PipelineStageDesc>& pipelineStageDescriptions);

		uint32 CreateShaderStageMask(const RenderStageDesc* pRenderStageDesc);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		IDescriptorHeap*									m_pDescriptorHeap;

		PipelineStage*										m_pPipelineStages;
		RenderStage*										m_pRenderStages;
		SynchronizationStage*								m_pSynchronizationStages;

		std::unordered_map<const char*, Resource>			m_ResourceMap;
	};
}