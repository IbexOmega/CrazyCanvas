#include "RenderStages/PlayerRenderer.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/EntityMaskManager.h"

#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"


namespace LambdaEngine
{

	PlayerRenderer* PlayerRenderer::s_pInstance = nullptr;

	PlayerRenderer::PlayerRenderer()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;
	}

	PlayerRenderer::~PlayerRenderer()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;

		if (m_ppGraphicCommandAllocators != nullptr && m_ppGraphicCommandLists != nullptr)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppGraphicCommandLists[b]);
				SAFERELEASE(m_ppGraphicCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppGraphicCommandLists);
			SAFEDELETE_ARRAY(m_ppGraphicCommandAllocators);
		}
	}

	bool PlayerRenderer::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;

		m_UsingMeshShader = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[PlayerRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[PlayerRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[PlayerRenderer]: Failed to create Shaders");
			return false;
		}

		return true;
	}

	bool PlayerRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->pDepthStencilAttachmentDesc != nullptr);

		if (!m_Initilized)
		{
			if (!CreateCommandLists())
			{
				LOG_ERROR("[PlayerRenderer]: Failed to create render command lists");
				return false;
			}

			if (!CreateRenderPass(pPreInitDesc->pColorAttachmentDesc, pPreInitDesc->pDepthStencilAttachmentDesc))
			{
				LOG_ERROR("[PlayerRenderer]: Failed to create RenderPass");
				return false;
			}

			if (!CreatePipelineState())
			{
				LOG_ERROR("[PlayerRenderer]: Failed to create PipelineState");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}

	void PlayerRenderer::Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);

		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);
		m_CurrModFrameIndex = modFrameIndex;
	}

	void PlayerRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		// Fetching render targets
		if (resourceName == "INTERMEDIATE_OUTPUT_IMAGE") 
		{
			if (imageCount != 0)
			{
				LOG_WARNING("RenderGraph has been altered. imageCount is %d but should be 0", imageCount);
			}
			m_IntermediateOutputImage = MakeSharedRef(ppPerImageTextureViews[0]);
		}

		if (resourceName == "G_BUFFER_DEPTH_STENCIL")
		{
			for (uint32 i = 0; i < imageCount; i++)
			{
				// Not sure if this the correct textureView
				m_DepthStencil = MakeSharedRef(ppPerImageTextureViews[0]);
			}
		}
		
		// Writing textures to DescriptorSets
		if (resourceName == SCENE_ALBEDO_MAPS)
		{
			constexpr DescriptorSetIndex setIndex = 1U;
			
			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* sampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 0;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] SCENE_ALBEDO_MAPS", setIndex);
			}
		}
		else if (resourceName == SCENE_NORMAL_MAPS)
		{
			constexpr DescriptorSetIndex setIndex = 1U;
			
			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* sampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 1;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] SCENE_NORMAL_MAPS", setIndex);
			}
		}
		else if (resourceName == SCENE_COMBINED_MATERIAL_MAPS)
		{
			constexpr DescriptorSetIndex setIndex = 1U;
			
			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* sampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 2;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] SCENE_COMBINED_MATERIAL_MAPS", setIndex);
			}
		}
	}

	void PlayerRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(ppBuffers);
		// create the descriptors that we described in CreatePipelineLayout()

		if (resourceName == SCENE_MAT_PARAM_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 0U;
			
			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet0 != nullptr)
			{
				m_DescriptorSet0->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					1,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] SCENE_MAT_PARAM_BUFFER", setIndex);
			}
		}
		else if (resourceName == PAINT_MASK_COLORS)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet0 != nullptr)
			{
				m_DescriptorSet0->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					2,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] PAINT_MASK_COLORS", setIndex);
			}
		}

		if (resourceName == PER_FRAME_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet0 != nullptr)
			{
				m_DescriptorSet0->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					0,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d] PER_FRAME_BUFFER", setIndex);
			}
		}

	}

	void PlayerRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		if (resourceName == SCENE_DRAW_ARGS)
		{
			if (count > 0U && pDrawArgs != nullptr)
			{
				m_pDrawArgs = pDrawArgs;
				m_DrawCount = count;

				m_DescriptorSetList2.Clear();
				m_DescriptorSetList2.Resize(m_DrawCount);
				m_DescriptorSetList3.Clear();
				m_DescriptorSetList3.Resize(m_DrawCount);
				
				// ---------------- START: Not sure what it is used for? -------------------
				// m_DrawCount is telling us how many times to draw per drawcall?
				// should not be more than 1 since there is only one player type we are filtering on.

				m_DirtyUniformBuffers = true;

				ECSCore* pECSCore = ECSCore::GetInstance();
				const ComponentArray<TeamComponent>* pTeamComponents = pECSCore->GetComponentArray<TeamComponent>();
				const ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECSCore->GetComponentArray<PlayerLocalComponent>();

				m_ViewerTeamId = MAXUINT32;
				m_TeamIds.Clear(); // All players which are not the viewer.
				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					constexpr DescriptorSetIndex setIndex = 2U;

					// Create a new descriptor or use an old descriptor
					m_DescriptorSetList2[d] = m_DescriptorCache.GetDescriptorSet("Player Renderer Descriptor Set 2 - Draw arg-" + std::to_string(d), m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

					if (m_DescriptorSetList2[d] != nullptr)
					{
						// Assume EntityIDs is always 1 in length. (Because animated meshes.)
						Entity entity = m_pDrawArgs[d].EntityIDs[0];
						if (pTeamComponents->HasComponent(entity))
						{
							if (pPlayerLocalComponents && pPlayerLocalComponents->HasComponent(entity))
							{
								TeamComponent teamComp = pTeamComponents->GetConstData(entity);
								m_ViewerTeamId = teamComp.TeamIndex;
								m_ViewerDrawArgIndex = d;
							}
							else
							{
								TeamComponent teamComp = pTeamComponents->GetConstData(entity);
								m_TeamIds.PushBack(teamComp.TeamIndex);

								// Set Vertex and Instance buffer for rendering
								Buffer* ppBuffers[2] = { m_pDrawArgs[d].pVertexBuffer, m_pDrawArgs[d].pInstanceBuffer };
								uint64 pOffsets[2] = { 0, 0 };
								uint64 pSizes[2] = { m_pDrawArgs[d].pVertexBuffer->GetDesc().SizeInBytes, m_pDrawArgs[d].pInstanceBuffer->GetDesc().SizeInBytes };

								m_DescriptorSetList2[d]->WriteBufferDescriptors(
									ppBuffers,
									pOffsets,
									pSizes,
									0,
									2,
									EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
								);
							}
						}
						else
						{
							LOG_ERROR("[PlayerRenderer]: A entity must have a TeamComponent for it to be processed by PlayerRenderer!");
						}
					}
					else
					{
						LOG_ERROR("[PlayerRenderer]: Failed to update descriptors for drawArgs vertices and instance buffers");
					}
				}

				std::transform(m_TeamIds.Begin(), m_TeamIds.End(), m_TeamIds.Begin(),
				[&](uint32& teamId)->uint32 {
					if (teamId == m_ViewerTeamId)
						return 0;
					return 1;
				});

				// Get Paint Mask Texture from each player
				for (uint32 d = 0; d < count; d++)
				{
					constexpr DescriptorSetIndex setIndex = 3U;

					// Create a new descriptor or use an old descriptor
					m_DescriptorSetList3[d] = m_DescriptorCache.GetDescriptorSet("Player Renderer Descriptor Set 3 - Draw arg-" + std::to_string(d), m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

					if (m_DescriptorSetList3[d] != nullptr)
					{
						const DrawArg& drawArg = pDrawArgs[d];

						TArray<TextureView*> textureViews;
						TextureView* defaultMask = ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_MASK_MAP);
						textureViews.PushBack(defaultMask);

						for (uint32 i = 0; i < drawArg.InstanceCount; i++)
						{
							DrawArgExtensionGroup* extensionGroup = drawArg.ppExtensionGroups[i];

							if (extensionGroup)
							{
								// We can assume there is only one extension, because this render stage has a DrawArgMask of 2 which is one specific extension.
								uint32 numExtensions = extensionGroup->ExtensionCount;
								for (uint32 e = 0; e < numExtensions; e++)
								{
									uint32 mask = extensionGroup->pExtensionMasks[e];
									bool inverted;
									uint32 meshPaintBit = EntityMaskManager::GetExtensionMask(MeshPaintComponent::Type(), inverted);
									uint32 invertedUInt = uint32(inverted);

									if ((mask & meshPaintBit) != invertedUInt)
									{
										DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
										TextureView* pTextureView = extension.ppMipZeroTextureViews[0];
										textureViews.PushBack(pTextureView);
									}
								}
							}
						}

						// Set descriptor to give GPU access to paint mask textures
						Sampler* sampler = Sampler::GetNearestSampler();
						uint32 bindingIndex = 0;

						m_DescriptorSetList3[d]->WriteTextureDescriptors(
								textureViews.GetData(),
								&sampler,
								ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, textureViews.GetSize(),
								EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
								false
							);
					}
					else
					{
						LOG_ERROR("[PlayerRenderer]: Failed to update descriptors for drawArgs paint masks");
					}
				}

			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update descriptors for drawArgs");
			}
		}
	}

	void PlayerRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (Sleeping)
			return;

		CommandList* pCommandList = m_ppGraphicCommandLists[modFrameIndex];

		uint32 width = m_IntermediateOutputImage->GetDesc().pTexture->GetDesc().Width;
		uint32 height = m_IntermediateOutputImage->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets = m_IntermediateOutputImage.GetAddressOf();
		beginRenderPassDesc.pDepthStencil = m_DepthStencil.Get();
		beginRenderPassDesc.RenderTargetCount = 1;
		beginRenderPassDesc.Width = width;
		beginRenderPassDesc.Height = height;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = nullptr;
		beginRenderPassDesc.ClearColorCount = 0;
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;

		if (m_DrawCount == 0)
		{
			m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
			//Begin and End RenderPass to transition Texture State (Lazy)
			pCommandList->BeginRenderPass(&beginRenderPassDesc);
			pCommandList->EndRenderPass();

			pCommandList->End();

			(*ppFirstExecutionStage) = pCommandList;
			return;
		}

		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet0.Get(), m_PipelineLayout.Get(), 0);
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet1.Get(), m_PipelineLayout.Get(), 1);

		pCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.Width = (float32)width;
		viewport.Height = -(float32)height;
		viewport.x = 0.0f;
		viewport.y = (float32)height;
		pCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width = width;
		scissorRect.Height = height;
		pCommandList->SetScissorRects(&scissorRect, 0, 1);

		for (uint32 d = 0, teamIndex = 0; d < m_DrawCount; d++)
		{
			if (d != m_ViewerDrawArgIndex)
			{
				const DrawArg& drawArg = m_pDrawArgs[d];

				uint32 teamId = m_TeamIds[teamIndex++];
				pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, &teamId, sizeof(uint32), 0);

				pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

				pCommandList->BindDescriptorSetGraphics(m_DescriptorSetList2[d].Get(), m_PipelineLayout.Get(), 2); // Mesh data (Vertices and instance buffers)
				pCommandList->BindDescriptorSetGraphics(m_DescriptorSetList3[d].Get(), m_PipelineLayout.Get(), 3); // Paint Masks

				pCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
			}
		}
		pCommandList->EndRenderPass();


		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}



	bool PlayerRenderer::CreatePipelineLayout()
	{
		/* VERTEX SHADER */
		// PerFrameBuffer
		DescriptorBindingDesc perFrameBufferDesc = {};
		perFrameBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount = 1;
		perFrameBufferDesc.Binding = 0;
		perFrameBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc verticesBindingDesc = {};
		verticesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		verticesBindingDesc.DescriptorCount = 1;
		verticesBindingDesc.Binding = 0;
		verticesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc instanceBindingDesc = {};
		instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc.DescriptorCount = 1;
		instanceBindingDesc.Binding = 1;
		instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		/* PIXEL SHADER */
		// MaterialParameters
		DescriptorBindingDesc materialParametersBufferDesc = {};
		materialParametersBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		materialParametersBufferDesc.DescriptorCount = 1;
		materialParametersBufferDesc.Binding = 1;
		materialParametersBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc paintMaskColorsBufferDesc = {};
		paintMaskColorsBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		paintMaskColorsBufferDesc.DescriptorCount = 1;
		paintMaskColorsBufferDesc.Binding = 2;
		paintMaskColorsBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// u_AlbedoMaps
		DescriptorBindingDesc albedoMapsDesc = {};
		albedoMapsDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		albedoMapsDesc.DescriptorCount = 6000;
		albedoMapsDesc.Binding = 0;
		albedoMapsDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		albedoMapsDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

		// NormalMapsDesc
		DescriptorBindingDesc normalMapsDesc = {};
		normalMapsDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		normalMapsDesc.DescriptorCount = 6000;
		normalMapsDesc.Binding = 1;
		normalMapsDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		normalMapsDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

		// CombinedMaterialMaps
		DescriptorBindingDesc combinedMaterialMapsDesc = {};
		combinedMaterialMapsDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		combinedMaterialMapsDesc.DescriptorCount = 6000;
		combinedMaterialMapsDesc.Binding = 2;
		combinedMaterialMapsDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		combinedMaterialMapsDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

		// PaintMaskTextures
		DescriptorBindingDesc paintMaskDesc = {};
		paintMaskDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		paintMaskDesc.DescriptorCount = 6000;
		paintMaskDesc.Binding = 0;
		paintMaskDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		paintMaskDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;


		// maps to SET = 0 (BUFFER_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { perFrameBufferDesc, materialParametersBufferDesc, paintMaskColorsBufferDesc };

		// maps to SET = 1 (TEXTURE_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { albedoMapsDesc, normalMapsDesc, combinedMaterialMapsDesc };

		// maps to SET = 2 (DRAW_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc };

		// maps to SET = 3 (DRAW_EXTENSION_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc3 = {};
		descriptorSetLayoutDesc3.DescriptorBindings = { paintMaskDesc };

		ConstantRangeDesc constantRangeFragmentDesc = { };
		constantRangeFragmentDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		constantRangeFragmentDesc.SizeInBytes = sizeof(uint32);
		constantRangeFragmentDesc.OffsetInBytes = 0;

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Player Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2, descriptorSetLayoutDesc3 };
		pipelineLayoutDesc.ConstantRanges = { constantRangeFragmentDesc };
		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool PlayerRenderer::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 4;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 4;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Player Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 512;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
	}

	bool PlayerRenderer::CreateShaders()
	{
		m_VertexShaderPointGUID = ResourceManager::LoadShaderFromFile("/Players/Players.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderPointGUID = ResourceManager::LoadShaderFromFile("/Players/Players.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderPointGUID != GUID_NONE && m_PixelShaderPointGUID != GUID_NONE;
	}

	bool PlayerRenderer::CreateCommandLists()
	{
		m_ppGraphicCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppGraphicCommandLists		 = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppGraphicCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Player Renderer Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppGraphicCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Player Renderer Graphics Command List " + std::to_string(b);
			commandListDesc.CommandListType = ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags = FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppGraphicCommandLists[b] = RenderAPI::GetDevice()->CreateCommandList(m_ppGraphicCommandAllocators[b], &commandListDesc);

			if (!m_ppGraphicCommandLists[b])
			{
				return false;
			}
		}

		return true;
	}

	bool PlayerRenderer::CreateRenderPass(RenderPassAttachmentDesc* pColorAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc)
	{
		// explain state of incoming texture

		// CHANGE: get one color attachment, one depth attachment
		// CHANGE: parameters to get the incomcing textures

		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format = pColorAttachmentDesc->Format; //VK_FORMAT_R8G8B8A8_UNORM
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState = pColorAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState = pColorAttachmentDesc->FinalState;

		RenderPassAttachmentDesc depthAttachmentDesc = {};
		
		depthAttachmentDesc.Format = pDepthStencilAttachmentDesc->Format; // FORMAT_D24_UNORM_S8_UINT
		depthAttachmentDesc.SampleCount = 1;
		depthAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		depthAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		depthAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		depthAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		depthAttachmentDesc.InitialState = pDepthStencilAttachmentDesc->InitialState;
		depthAttachmentDesc.FinalState = pDepthStencilAttachmentDesc->FinalState;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET }; // specify render targets state
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT; // special case for depth

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass = EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass = 0;
		subpassDependencyDesc.SrcAccessMask = 0;
		subpassDependencyDesc.DstAccessMask = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName = "Player Renderer Render Pass";
		renderPassDesc.Attachments = { colorAttachmentDesc, depthAttachmentDesc };
		renderPassDesc.Subpasses = { subpassDesc };
		renderPassDesc.SubpassDependencies = { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool PlayerRenderer::CreatePipelineState()
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName = "Player Renderer Pipeline State";
		pipelineStateDesc.RenderPass = m_RenderPass;
		pipelineStateDesc.PipelineLayout = m_PipelineLayout;

		pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
		pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_NONE;

		pipelineStateDesc.DepthStencilState = {};
		pipelineStateDesc.DepthStencilState.DepthTestEnable = true;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable = true;

		pipelineStateDesc.BlendState.BlendAttachmentStates =
		{
			{
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendOp::BLEND_OP_ADD,
				EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A,
				true
			}
		};

		pipelineStateDesc.VertexShader.ShaderGUID = m_VertexShaderPointGUID;
		pipelineStateDesc.PixelShader.ShaderGUID = m_PixelShaderPointGUID;

		m_PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		return true;
	}
}