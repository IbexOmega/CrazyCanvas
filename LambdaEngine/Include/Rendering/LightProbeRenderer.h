#pragma once
#include "Rendering/CustomRenderer.h"

#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineLayout.h"

namespace LambdaEngine
{
	class LightProbeRenderer : public CustomRenderer
	{
	public:
		LightProbeRenderer();
		~LightProbeRenderer() = default;

		virtual bool Init() override final;

		virtual void UpdateTextureResource(
			const String& resourceName,
			const TextureView* const* ppPerImageTextureViews,
			const TextureView* const* ppPerSubImageTextureViews,
			const Sampler* const* ppPerImageSamplers,
			uint32 imageCount,
			uint32 subImageCount,
			bool backBufferBound) override final;

		virtual void Render(
			uint32 modFrameIndex,
			uint32 backBufferIndex,
			CommandList** ppFirstExecutionStage,
			CommandList** ppSecondaryExecutionStage,
			bool sleeping) override final;

		FORCEINLINE virtual FPipelineStageFlag GetFirstPipelineStage() const override final 
		{ 
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; 
		}

		FORCEINLINE virtual FPipelineStageFlag GetLastPipelineStage() const override final 
		{ 
			return FPipelineStageFlag::PIPELINE_STAGE_FLAG_COMPUTE_SHADER; 
		}

		FORCEINLINE virtual const String& GetName() const override final
		{
			static String name = "LIGHT_PROBE_RENDERER";
			return name;
		}

	private:
		const Texture*		m_pEnvironmentMap		= nullptr;
		const TextureView*	m_pEnvironmentMapView	= nullptr;
		const Texture*		m_pGlobalDiffuse		= nullptr;
		const TextureView*	m_pGlobalDiffuseView	= nullptr;
		const Texture*		m_pGlobalSpecular		= nullptr;
		const TextureView*	m_pGlobalSpecularView	= nullptr;
		TArray<const TextureView*> m_GlobalSpecularWriteViews;
		
		bool m_NeedsUpdate = true;

		TArray<TSharedRef<CommandList>>			m_ComputeCommandLists;
		TArray<TSharedRef<CommandAllocator>>	m_ComputeCommandAllocators;

		uint64 m_SpecularFilterState;
		TSharedRef<PipelineLayout>	m_SpecularFilterLayout;
		uint64 m_DiffuseFilterState;
		TSharedRef<PipelineLayout>	m_DiffuseFilterLayout;

		TSharedRef<DescriptorHeap> m_FilterDescriptorHeap;
		TArray<TSharedRef<DescriptorSet>> m_SpecularFilterDescriptorSets;
		TArray<TSharedRef<DescriptorSet>> m_DiffuseFilterDescriptorSets;
	};
}