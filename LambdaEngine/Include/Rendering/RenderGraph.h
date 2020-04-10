#pragma once

#include "LambdaEngine.h"

#include "RenderGraphTypes.h"

#include <unordered_map>

namespace LambdaEngine
{
	struct TextureDesc;
	struct SamplerDesc;
	struct BufferDesc;

	class IGraphicsDevice;
	class IPipelineLayout;
	class IPipelineState;
	class IDescriptorSet;
	class IDescriptorHeap;
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
				ISampler* pSamplerDesc;
			} ExternalTextureUpdate;

			struct
			{
				IBuffer* pBuffer;
			} ExternalBufferUpdate;
		};
	};

	class LAMBDA_API RenderGraph
	{
		enum class EResourceType
		{
			UNKNOWN				= 0,
			INTERNAL_TEXTURE	= 1,
			INTERNAL_BUFFER		= 2,
			EXTERNAL_TEXTURE	= 3,
			EXTERNAL_BUFFER		= 4,
			EXTERNAL_UNIFORM	= 5,
		};

		struct ResourceBinding
		{
			IDescriptorSet* pDescriptorSet	= nullptr;
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
					ITexture* pTexture		= nullptr;
					ISampler* pSamplerDesc	= nullptr;
				} Texture;

				struct
				{
					IBuffer* pBuffer		= nullptr;
				} Buffer;
			};
		};

		struct RenderStage
		{
			IPipelineLayout*	pPipelineLayout		= nullptr;
			IPipelineState*		pPipelineState		= nullptr;
			IDescriptorSet*		pDescriptorSet		= nullptr;
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
		bool CreateRenderStages(const std::vector<RenderStageDesc>& renderStageDescriptions);

	private:
		const IGraphicsDevice*								m_pGraphicsDevice;

		IDescriptorHeap*									m_pDescriptorHeap;

		RenderStage*										m_pRenderStages;
		uint32												m_RenderStageCount;

		std::unordered_map<const char*, Resource*>			m_Resources;
	};
}