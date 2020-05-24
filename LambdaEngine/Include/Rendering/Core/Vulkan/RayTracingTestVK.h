#pragma once

#include "LambdaEngine.h"
#include "AccelerationStructureVK.h"

namespace LambdaEngine
{
	struct BuildBottomLevelAccelerationStructureDesc;
	struct BuildTopLevelAccelerationStructureDesc;

	class CommandAllocatorVK;
	class CommandListVK;
	class ITextureView;
	class IPipelineState;
	class IPipelineLayout;
	class IFence;
	class IDescriptorHeap;
	class IDescriptorSet;

	class RayTracingTestVK
	{
	public:
		DECL_STATIC_CLASS(RayTracingTestVK);

		static void InitCommandLists();
		static void InitRenderer(ITextureView** ppBackBufferTextureViews, GUID_Lambda raygenShader, GUID_Lambda closestHitShader, GUID_Lambda missShader);

		static void CreateBLAS();
		static void BuildBLAS();
		static void CreateTLAS();

		static void GetAndIncrementFence(IFence** ppFence, uint64* pSignalValue);

		static void Render(uint64 modFrameIndex, uint32 backBufferIndex);

		static void Debug(IAccelerationStructure* pBLAS, IAccelerationStructure* pTLAS);

	private:
		static AccelerationStructureVK*			s_pTLAS;
		static AccelerationStructureVK*			s_pBLAS;

		static BufferVK*						s_pBLASSerializedBuffer;
		static BufferVK*						s_pTLASSerializedBuffer;

		static IFence*							s_pFence;
		static uint64							s_SignalValue;

		static ITextureView*					s_ppTextureViews[3];

		static CommandAllocatorVK*				s_pGraphicsPreCommandAllocators[3];
		static CommandAllocatorVK*				s_pGraphicsPostCommandAllocators[3];
		static CommandAllocatorVK*				s_pComputeCommandAllocators[3];

		static CommandListVK*					s_pGraphicsPreCommandLists[3];
		static CommandListVK*					s_pGraphicsPostCommandLists[3];
		static CommandListVK*					s_pComputeCommandLists[3];

		static IPipelineState*					s_pPipelineState;
		static IPipelineLayout*					s_pPipelineLayout;

		static IDescriptorHeap*					s_pDescriptorHeap;
		static IDescriptorSet*					s_pDescriptorSets[3];

		static VkPipeline						s_Pipeline;
		static VkPipelineLayout					s_PipelineLayout;
		static VkDescriptorSetLayout			s_DescriptorSetLayout;
		static BufferVK*						s_pSBT;

		static VkDescriptorPool					s_DescriptorPool;
		static VkDescriptorSet					s_DescriptorSets[3];
	};
}