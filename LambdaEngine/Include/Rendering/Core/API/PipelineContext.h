#pragma once

#include "Time/API/Timestamp.h"

#include "Rendering/RenderGraphTypes.h"

#include "Rendering/Core/API/DescriptorCache.h"

namespace LambdaEngine
{
	struct SDescriptorBufferUpdateDesc
	{
		const Buffer* const* ppBuffers;
		const uint64* pOffsets; 
		const uint64* pSizes; 
		uint32 FirstBinding;
		uint32 DescriptorCount;
		EDescriptorType DescriptorType;
	};

	struct SDescriptorTextureUpdateDesc
	{
		const TextureView* const* ppTextures;
		const Sampler* const* ppSamplers;
		ETextureState TextureState;
		uint32 FirstBinding;
		uint32 DescriptorCount;
		EDescriptorType DescriptorType;
		bool UniqueSamplers;
	};

	using SetIndex = uint32;

	class PipelineContext
	{
	public:
		PipelineContext() = default;
		~PipelineContext() = default;

		/*
		*	All Descriptor set layouts and shaders has to be added before calling init 
		*/
		bool Init(const String& debugname);

		void Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex);

		void Bind(CommandList* pCommandList);

		void BindConstantRange(CommandList* pCommandList, void* pPushConstantData, uint32 pushConstantSize, uint32 pushConstantOffset);

		/*
		*	Set Index will be determined by order of call of this function
		*/
		void CreateDescriptorSetLayout(const TArray<DescriptorBindingDesc>& descriptorBindings);

		void CreateConstantRange(const ConstantRangeDesc& constantRangeDesc);

		void UpdateDescriptorSet(const String& debugname, uint32 setIndex, DescriptorHeap* pDescriptorHeap, const SDescriptorBufferUpdateDesc& descriptorUpdateDesc);
		void UpdateDescriptorSet(const String& debugname, uint32 setIndex, DescriptorHeap* pDescriptorHeap, const SDescriptorTextureUpdateDesc& descriptorUpdateDesc);

		void SetVertexShader(GUID_Lambda vertexShader) { m_VertexShaderGUID = vertexShader; };
		void SetFragmentShader(GUID_Lambda fragmentShader) { m_FragmentShaderGUID = fragmentShader; };
		void SetComputeShader(GUID_Lambda computeShader) { m_ComputeShaderGUID = computeShader; };

		GUID_Lambda GetVertexShader() { return m_VertexShaderGUID; };
		GUID_Lambda GetFragmentShader() { return m_FragmentShaderGUID; };
		GUID_Lambda GetComputeShader() { return m_ComputeShaderGUID; };

	private:
		bool									m_Initilized = false;

		GUID_Lambda								m_VertexShaderGUID = GUID_NONE;
		GUID_Lambda								m_FragmentShaderGUID = GUID_NONE;
		GUID_Lambda								m_ComputeShaderGUID = GUID_NONE;

		uint64									m_PipelineStateID = 0;
		TSharedRef<PipelineLayout>				m_PipelineLayout = nullptr;

		TArray<ConstantRangeDesc>				m_ConstantRangeDescs;
		TArray<DescriptorSetLayoutDesc>			m_DescriptorSetLayoutDescs;

		// Descriptor sets
		THashTable<SetIndex, TSharedRef<DescriptorSet>>		m_DescriptorSets;
		DescriptorCache										m_DescriptorCache;

	};
}