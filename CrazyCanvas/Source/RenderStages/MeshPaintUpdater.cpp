#include "RenderStages/MeshPaintUpdater.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	std::unordered_set<Entity> MeshPaintUpdater::s_EntitiesToClear;

	MeshPaintUpdater::MeshPaintUpdater()
	{
	}

	MeshPaintUpdater::~MeshPaintUpdater()
	{
		if (m_Initilized)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppComputeCommandLists[b]);
				SAFERELEASE(m_ppComputeCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppComputeCommandLists);
			SAFEDELETE_ARRAY(m_ppComputeCommandAllocators);

			if (m_Sampler)
				SAFERELEASE(m_Sampler);
		}
	}

	void MeshPaintUpdater::ClearServer(Entity entity)
	{
		s_EntitiesToClear.insert(entity);
	}

	bool LambdaEngine::MeshPaintUpdater::CreatePipelineLayout()
	{
		// Set 0
		{
			DescriptorBindingDesc hitPointsBindingDesc = {};
			hitPointsBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
			hitPointsBindingDesc.DescriptorCount = 1;
			hitPointsBindingDesc.Binding = 0;
			hitPointsBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			TArray<DescriptorBindingDesc> descriptorBindings = {
				hitPointsBindingDesc
			};

			m_UpdatePipeline.CreateDescriptorSetLayout(descriptorBindings);
		}

		// Set 1
		{
			DescriptorBindingDesc verticesBindingDesc = {};
			verticesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			verticesBindingDesc.DescriptorCount = 1;
			verticesBindingDesc.Binding = 0;
			verticesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			DescriptorBindingDesc instanceBindingDesc = {};
			instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			instanceBindingDesc.DescriptorCount = 1;
			instanceBindingDesc.Binding = 1;
			instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			TArray<DescriptorBindingDesc> descriptorBindings = {
				verticesBindingDesc,
				instanceBindingDesc
			};
			
			m_UpdatePipeline.CreateDescriptorSetLayout(descriptorBindings);
		}

		// Set 2
		{
			DescriptorBindingDesc brushMaskBindingDesc = {};
			brushMaskBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
			brushMaskBindingDesc.DescriptorCount = 1;
			brushMaskBindingDesc.Binding = 0;
			brushMaskBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;

			TArray<DescriptorBindingDesc> descriptorBindings = {
				brushMaskBindingDesc
			};

			m_UpdatePipeline.CreateDescriptorSetLayout(descriptorBindings);
		}

		ConstantRangeDesc constantRange = {};
		constantRange.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER;
		constantRange.SizeInBytes = sizeof(SPushConstantData);
		constantRange.OffsetInBytes = 0;

		m_UpdatePipeline.CreateConstantRange(constantRange);

		return true;
	}

	bool LambdaEngine::MeshPaintUpdater::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 1;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 2;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Mesh paint Updater Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 256;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
	}

	bool LambdaEngine::MeshPaintUpdater::CreateShaders()
	{
		bool success = true;

		String computeShaderFileName = "MeshPainting/MeshPaintUpdater.comp";
		GUID_Lambda computeShaderGUID = ResourceManager::LoadShaderFromFile(computeShaderFileName, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);
		success &= computeShaderGUID != GUID_NONE;

		m_UpdatePipeline.SetComputeShader(computeShaderGUID);

		return success;
	}

	bool LambdaEngine::MeshPaintUpdater::CreateCommandLists()
	{
		m_ppComputeCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppComputeCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppComputeCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Mesh Paint Updater Compute Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_COMPUTE);

			if (!m_ppComputeCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Mesh Paint Updater Compute Command List " + std::to_string(b);
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppComputeCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppComputeCommandAllocators[b], &commandListDesc);

			if (!m_ppComputeCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool LambdaEngine::MeshPaintUpdater::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;
		m_ResourcesToRemove.Resize(m_BackBufferCount);

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[MeshPaintUpdater]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[MeshPaintUpdater]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[MeshPaintUpdater]: Failed to create Shaders");
			return false;
		}

		return true;
	}

	bool LambdaEngine::MeshPaintUpdater::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		if (!m_Initilized)
		{
			if (!CreateCommandLists())
			{
				LOG_ERROR("[MeshPaintUpdater]: Failed to create render command lists");
				return false;
			}

			if (!m_UpdatePipeline.Init("MeshPaintUpdater Compute"))
			{
				LOG_ERROR("[MeshPaintUpdater]: Failed to init Updater Pipeline Context");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}

	void MeshPaintUpdater::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);

		m_UpdatePipeline.Update(delta, modFrameIndex, backBufferIndex);
	}

	void MeshPaintUpdater::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* const* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void MeshPaintUpdater::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "BRUSH_MASK")
		{
			constexpr uint32 setIndex = 2U;
			constexpr uint32 setBinding = 0U;

			Sampler* pSampler = Sampler::GetLinearSampler();

			SDescriptorTextureUpdateDesc descriptorUpdateDesc = {};
			descriptorUpdateDesc.ppTextures = ppPerImageTextureViews;
			descriptorUpdateDesc.ppSamplers = &pSampler;
			descriptorUpdateDesc.TextureState = ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			descriptorUpdateDesc.FirstBinding = setBinding;
			descriptorUpdateDesc.UniqueSamplers = false;
			descriptorUpdateDesc.DescriptorCount = 1;
			descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;

			m_UpdatePipeline.UpdateDescriptorSet("[MeshPaintUpdater] Brush mask texture Descriptor Set 2 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc);
		}
	}

	void MeshPaintUpdater::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "HIT_POINTS_BUFFER")
		{
			constexpr uint32 setIndex = 0U;
			constexpr uint32 setBinding = 0U;

			SDescriptorBufferUpdateDesc descriptorUpdateDesc = {};
			descriptorUpdateDesc.ppBuffers = ppBuffers;
			descriptorUpdateDesc.pOffsets = pOffsets;
			descriptorUpdateDesc.pSizes = pSizesInBytes;
			descriptorUpdateDesc.FirstBinding = setBinding;
			descriptorUpdateDesc.DescriptorCount = count;
			descriptorUpdateDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;

			m_UpdatePipeline.UpdateDescriptorSet("[MeshPaintUpdater] Hit points Buffer Descriptor Set 0 Binding 0", setIndex, m_DescriptorHeap.Get(), descriptorUpdateDesc, false);
		}
	}

	void MeshPaintUpdater::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		if (resourceName == SCENE_DRAW_ARGS)
		{
			constexpr uint32 setIndex = 1U;
			constexpr uint32 setBinding = 0U;

			m_VertexCountList.Clear();

			// Remove previous draw args
			for (std::pair<Entity, TSharedRef<DescriptorSet>> desc : m_DrawArgDescriptorSets)
			{
				m_ResourcesToRemove[m_CurrentFrameIndex].PushBack(desc.second.GetAndAddRef());
			}
			m_DrawArgDescriptorSets.Clear();

			for (uint32 d = 0; d < count; d++)
			{
				const DrawArg& drawArg = pDrawArgs[d];
				uint32 vertexCount = (uint32)(drawArg.pVertexBuffer->GetDesc().SizeInBytes / sizeof(Vertex));
				m_VertexCountList.PushBack(vertexCount);

				TSharedRef<DescriptorSet> descriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet(
					"[MeshPaintUpdater] DrawArgs Buffer Descriptor Set 0 Binding 0 Index - " + std::to_string(d), 
					m_UpdatePipeline.GetPipelineLayout().Get(), setIndex, m_DescriptorHeap.Get());

				Buffer* ppBuffers[2] = { drawArg.pVertexBuffer, drawArg.pInstanceBuffer };
				uint64 pOffsets[2] = { 0, 0 };
				uint64 pSizes[2] = { drawArg.pVertexBuffer->GetDesc().SizeInBytes, drawArg.pInstanceBuffer->GetDesc().SizeInBytes };
				descriptorSet->WriteBufferDescriptors(ppBuffers, pOffsets, pSizes, setBinding, 2, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);

				m_DrawArgDescriptorSets.PushBack(std::make_pair(drawArg.EntityIDs[0], descriptorSet));
			}
		}
	}

	void MeshPaintUpdater::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		if (m_VertexCountList.IsEmpty())
			return;

		m_CurrentFrameIndex = modFrameIndex;

		// Remove old resources
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_CurrentFrameIndex];
		for (DeviceChild* resource : resourcesToRemove)
			resource->Release();
		resourcesToRemove.Clear();

		CommandList* pCommandList = m_ppComputeCommandLists[modFrameIndex];
		m_ppComputeCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		m_UpdatePipeline.Bind(pCommandList);

		for (uint32 d = 0; d < m_VertexCountList.GetSize(); d++)
		{
			SPushConstantData pushConstantData = {};
			pushConstantData.VertexCount = m_VertexCountList[d];
			// Check if this entity should clear its server side vertex paint.
			pushConstantData.ShouldResetServer = (uint32)s_EntitiesToClear.contains(m_DrawArgDescriptorSets[d].first);

			pCommandList->BindDescriptorSetCompute(m_DrawArgDescriptorSets[d].second.Get(), m_UpdatePipeline.GetPipelineLayout().Get(), 1);
			m_UpdatePipeline.BindConstantRange(pCommandList, (void*)&pushConstantData, sizeof(pushConstantData), 0U);

			constexpr uint32 WORK_GROUP_INVOCATIONS = 32;
			uint32 workGroupX = uint32(std::ceilf(float(pushConstantData.VertexCount) / float(WORK_GROUP_INVOCATIONS)));
			pCommandList->Dispatch(workGroupX, 1U, 1U);
		}

		pCommandList->End();

		s_EntitiesToClear.clear();

		(*ppFirstExecutionStage) = pCommandList;
	}
}