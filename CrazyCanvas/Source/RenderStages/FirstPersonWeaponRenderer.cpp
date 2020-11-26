#include "RenderStages/FirstPersonWeaponRenderer.h"
#include "Application/API/CommonApplication.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/EntityMaskManager.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/Core/API/GraphicsDevice.h"

#include "ECS/ECSCore.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"
#include "Game/ECS/Components/Player/PlayerRelatedComponent.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Physics/Transform.h"
#include "Engine/EngineConfig.h"


#include "Rendering/RenderGraph.h"
#include "ECS/Components/Player/WeaponComponent.h"
#include "Resources/ResourceCatalog.h"

namespace LambdaEngine
{
	FirstPersonWeaponRenderer* FirstPersonWeaponRenderer::s_pInstance = nullptr;

	FirstPersonWeaponRenderer::FirstPersonWeaponRenderer()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;
	}

	FirstPersonWeaponRenderer::~FirstPersonWeaponRenderer()
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

	bool FirstPersonWeaponRenderer::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;
		m_UsingMeshShader = EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);
		m_Entity = MAXUINT32;

		if (!CreateBuffers())
		{
			LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to create buffers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to create Shaders");
			return false;
		}

		return true;
	}

	bool FirstPersonWeaponRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		if (!m_Initilized)
		{
			m_pRenderGraph = pPreInitDesc->pRenderGraph;
			CommandList* pCommandList = m_pRenderGraph->AcquireGraphicsCopyCommandList();
			if (!PrepareResources(pCommandList))
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to create resources");
				return false;
			}

			if (!CreateCommandLists())
			{
				LOG_ERROR("[FirstPersonWeapoRenderer]: Failed to create render command lists");
				return false;
			}

			if (!CreateRenderPass(pPreInitDesc->pColorAttachmentDesc))
			{
				LOG_ERROR("[FirstPersonWeapoRenderer]: Failed to create RenderPass");
				return false;
			}

			if (!CreatePipelineState())
			{
				LOG_ERROR("[FirstPersonWeapoRenderer]: Failed to create PipelineState");
				return false;
			}

			m_Initilized = true;
		}

		return true;
	}

	bool FirstPersonWeaponRenderer::CreateBuffers()
	{
		bool succeded = true;
		
		// Per frame buffers
		{
			BufferDesc stagingBufferDesc = {};
			stagingBufferDesc.DebugName = "FirstPersonWeapon Renderer Uniform Copy Buffer";
			stagingBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			stagingBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			stagingBufferDesc.SizeInBytes = sizeof(FrameBuffer);
	
			m_FrameCopyBuffer = RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);
			succeded = m_FrameCopyBuffer != nullptr;

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon Renderer Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_FrameBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = m_FrameBuffer != nullptr;
		}

		Mesh* pMesh = ResourceManager::GetMesh(ResourceCatalog::WEAPON_FIRST_PERSON_MESH_GUID);
		// Weapon Vertex Buffer
		{

			BufferDesc stagingBufferDesc = {};
			stagingBufferDesc.DebugName = "FirstPersonWeapon Renderer Vertex Copy Buffer";
			stagingBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			stagingBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			stagingBufferDesc.SizeInBytes = sizeof(Vertex) * pMesh->Vertices.GetSize();

			m_VertexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);
			succeded = m_VertexStagingBuffer != nullptr;

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon Vertex Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_VertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = m_VertexBuffer != nullptr;
		}

		// Weapon Index Buffer
		{
			BufferDesc stagingBufferDesc = {};
			stagingBufferDesc.DebugName = "FirstPersonWeapon Renderer Index Copy Buffer";
			stagingBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			stagingBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			stagingBufferDesc.SizeInBytes = sizeof(MeshIndexType) * pMesh->Indices.GetSize();

			m_IndexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);
			succeded = m_IndexStagingBuffer != nullptr;

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon Index Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_INDEX_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_IndexBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = m_IndexBuffer != nullptr;
		}

		// Weapon Data Buffer
		{
			BufferDesc stagingBufferDesc = {};
			stagingBufferDesc.DebugName = "FirstPersonWeapon Renderer WeaponData Copy Buffer";
			stagingBufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			stagingBufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
			stagingBufferDesc.SizeInBytes = sizeof(SWeaponBuffer);

			m_WeaponStagingBuffers.Resize(m_BackBufferCount);
			for (uint32 b = 0; b < m_WeaponStagingBuffers.GetSize(); b++)
			{
				m_WeaponStagingBuffers[b] = RenderAPI::GetDevice()->CreateBuffer(&stagingBufferDesc);
				succeded = m_WeaponStagingBuffers[b] != nullptr;
			}

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon WeaponData Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_WeaponBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = m_WeaponBuffer != nullptr;
		}

		return succeded;
	}

	void FirstPersonWeaponRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);
		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);

		// Update descriptor of weapon buffer set
		if (!m_InitilizedWeaponBuffer) 
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			Buffer* ppBuffers = m_WeaponBuffer.Get();
			uint64 pOffsets = 0;
			uint64 pSizes = m_WeaponBuffer->GetDesc().SizeInBytes;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet0 != nullptr)
			{
				m_DescriptorSet0->WriteBufferDescriptors(
					&ppBuffers,
					&pOffsets,
					&pSizes,
					4,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_LIGHTS_BUFFER", setIndex);
			}

			m_InitilizedWeaponBuffer = true;
		}
	}

	void FirstPersonWeaponRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, const Sampler* const* ppPerImageSamplers, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerImageSamplers);
		UNREFERENCED_VARIABLE(imageCount);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		// Fetching render targets
		if (resourceName == "INTERMEDIATE_OUTPUT_IMAGE")
		{
			m_IntermediateOutputImage = MakeSharedRef(ppPerImageTextureViews[0]);
		}
		else if (resourceName == "FIRST_PERSON_WEAPON_DEPTH_STENCIL")
		{
			m_DepthStencil = MakeSharedRef(ppPerImageTextureViews[0]);
		}
		else if (resourceName == SCENE_ALBEDO_MAPS) // Writing textures to DescriptorSets
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 0;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_ALBEDO_MAPS", setIndex);
			}
		}
		else if (resourceName == SCENE_NORMAL_MAPS)
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 1;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_NORMAL_MAPS", setIndex);
			}
		}
		else if (resourceName == SCENE_COMBINED_MATERIAL_MAPS)
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 2;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_COMBINED_MATERIAL_MAPS", setIndex);
			}
		}
	}

	void FirstPersonWeaponRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(ppBuffers);

		// create the descriptors that we described in CreatePipelineLayout()
		if (resourceName == SCENE_MAT_PARAM_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
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
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_MAT_PARAM_BUFFER", setIndex);
			}
		}
		else if (resourceName == PAINT_MASK_COLORS)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
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
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] PAINT_MASK_COLORS", setIndex);
			}
		}
		else if (resourceName == SCENE_LIGHTS_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet0 != nullptr)
			{
				m_DescriptorSet0->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					3,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] SCENE_LIGHTS_BUFFER", setIndex);
			}
		}
	}

	void FirstPersonWeaponRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
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

				m_DirtyUniformBuffers = true;

				ECSCore* pECSCore = ECSCore::GetInstance();
				const ComponentArray<WeaponLocalComponent>* pWeaponLocalComponents = pECSCore->GetComponentArray<WeaponLocalComponent>();
				const ComponentArray<PositionComponent>* pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();

				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					m_DescriptorSetList2[d] = MakeSharedRef(pDrawArgs[d].pDescriptorSet);
				
					if (m_DescriptorSetList2[d] != nullptr)
					{
						for (uint32 i = 0; i < m_pDrawArgs[d].EntityIDs.GetSize(); i++)
						{
							m_Entity = m_pDrawArgs[d].EntityIDs[i];
							if (pWeaponLocalComponents->HasComponent(m_Entity)) {
							
								m_WorldPos = pPositionComponents->GetConstData(m_Entity).Position;

								// Set Vertex and Instance buffer for rendering
								Buffer* ppBuffers[2] = { m_VertexBuffer.Get(), m_pDrawArgs[d].pInstanceBuffer };
								uint64 pOffsets[2] = { 0, 0 };
								uint64 pSizes[2] = { m_VertexBuffer->GetDesc().SizeInBytes, m_pDrawArgs[d].pInstanceBuffer->GetDesc().SizeInBytes };

								m_DescriptorSetList2[d]->WriteBufferDescriptors(
									ppBuffers,
									pOffsets,
									pSizes,
									0,
									2,
									EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
								);

								// Set Frame Buffer
								const Buffer* ppBuffers2 = { m_FrameBuffer.Get() };
								const uint64 pOffsets2 = { 0 };
								uint64 pSizesInBytes2 = { sizeof(FrameBuffer) };
								DescriptorSetIndex setIndex0 = 0;
								m_DescriptorSet0 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex0, m_DescriptorHeap.Get());
								if (m_DescriptorSet0 != nullptr)
								{
									m_DescriptorSet0->WriteBufferDescriptors(
										&ppBuffers2,
										&pOffsets2,
										&pSizesInBytes2,
										setIndex0,
										1,
										EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER
									);
								}
							}
							else
							{
								LOG_ERROR("[FirstPersonWeaponRenderer]: A entity must have a WeaponLocalComponent for it to be processed by FirstPersonWeaponRenderer!");
							}
						}
					}
					else
					{
						LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update descriptors for drawArgs vertices and instance buffers");
					}
				}

				// Get Paint Mask Texture from each player & weapon
				for (uint32 d = 0; d < count; d++)
				{
					constexpr DescriptorSetIndex setIndex = 3U;

					// Create a new descriptor or use an old descriptor
					m_DescriptorSetList3[d] = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Descriptor Set 3 - Draw arg-" + std::to_string(d), m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

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
									uint32 flag = extensionGroup->pExtensionFlags[e];
									bool inverted;
									uint32 meshPaintFlag = EntityMaskManager::GetExtensionFlag(MeshPaintComponent::Type(), inverted);
									uint32 invertedUInt = uint32(inverted);

									if ((flag & meshPaintFlag) != invertedUInt)
									{
										DrawArgExtensionData& extension = extensionGroup->pExtensions[e];
										TextureView* pTextureView = extension.ppTextureViews[0];
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
						LOG_ERROR("[FirstPersonWeapon]: Failed to update descriptors for drawArgs paint masks");
					}
				}
			}
			else
			{
				LOG_ERROR("[FirstPersonWeapon]: Failed to update descriptors for drawArgs");
			}
		}
	}

	void FirstPersonWeaponRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		uint32 width = m_IntermediateOutputImage->GetDesc().pTexture->GetDesc().Width;
		uint32 height = m_IntermediateOutputImage->GetDesc().pTexture->GetDesc().Height;

		ClearColorDesc ccDesc[2];
		ccDesc[0].Depth = 1.0f;
		ccDesc[0].Stencil = 0;
		ccDesc[1].Depth = 1.0f;
		ccDesc[1].Stencil = 0;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets = m_IntermediateOutputImage.GetAddressOf();
		beginRenderPassDesc.pDepthStencil = m_DepthStencil.Get();
		beginRenderPassDesc.RenderTargetCount = 1;
		beginRenderPassDesc.Width = width;
		beginRenderPassDesc.Height = height;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = ccDesc;
		beginRenderPassDesc.ClearColorCount = 2; // clearValueCount must be greater than the largest attachment index in renderPass
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;

		Viewport viewport = {};
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.Width = (float32)width;
		viewport.Height = -(float32)height;
		viewport.x = 0.0f;
		viewport.y = (float32)height;

		ScissorRect scissorRect = {};
		scissorRect.Width = width;
		scissorRect.Height = height;

		CommandList* pCommandList = m_ppGraphicCommandLists[modFrameIndex];
		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		UpdateWeaponBuffer(pCommandList, modFrameIndex);

		pCommandList->BeginRenderPass(&beginRenderPassDesc);

		if (!Sleeping)
		{
			pCommandList->SetViewports(&viewport, 0, 1);
			pCommandList->SetScissorRects(&scissorRect, 0, 1);

			if (m_DrawCount > 0)
			{
				RenderCull(pCommandList, m_PipelineStateIDFrontCull);
				RenderCull(pCommandList, m_PipelineStateIDBackCull);
			}
		}

		pCommandList->EndRenderPass();
		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}

	void FirstPersonWeaponRenderer::RenderCull(CommandList* pCommandList, uint64& pipelineId)
	{
		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(pipelineId));
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet0.Get(), m_PipelineLayout.Get(), 0); // BUFFER_SET_INDEX
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet1.Get(), m_PipelineLayout.Get(), 1); // TEXTURE_SET_INDEX

		// Draw Weapon
		const DrawArg& drawArg = m_pDrawArgs[0];
		pCommandList->BindIndexBuffer(m_IndexBuffer.Get(), 0, EIndexType::INDEX_TYPE_UINT32);
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSetList2[0].Get(), m_PipelineLayout.Get(), 2); // Mesh data (Vertices and instance buffers)
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSetList3[0].Get(), m_PipelineLayout.Get(), 3); // Paint Masks
		pCommandList->DrawIndexInstanced(m_IndicesCount, drawArg.InstanceCount, 0, 0, 0);
	}

	void FirstPersonWeaponRenderer::UpdateWeaponBuffer(CommandList* pCommandList, uint32 modFrameIndex)
	{
		if (m_Entity != MAXUINT32) {
			const ComponentArray<PositionComponent>* pPositionComponents = ECSCore::GetInstance()->GetComponentArray<PositionComponent>();
			SWeaponBuffer data = {};

			data.Model = glm::translate(glm::vec3(-0.5f, -0.4f, -0.5f));
			data.Model = glm::scale(data.Model, glm::vec3(1.2f, 1.2f, 1.2f));
			data.PlayerPos = pPositionComponents->GetConstData(m_Entity).Position;

			Buffer* pStagingBuffer = m_WeaponStagingBuffers[modFrameIndex].Get();
			byte* pMapping = reinterpret_cast<byte*>(pStagingBuffer->Map());
			memcpy(pMapping, &data, sizeof(data));
			pStagingBuffer->Unmap();
			pCommandList->CopyBuffer(pStagingBuffer, 0, m_WeaponBuffer.Get(), 0, sizeof(SWeaponBuffer));
		}
	}

	bool FirstPersonWeaponRenderer::PrepareResources(CommandList* pCommandList)
	{
		Mesh* pMesh = ResourceManager::GetMesh(ResourceCatalog::WEAPON_FIRST_PERSON_MESH_GUID);

		// Copy vertices
		const TArray<Vertex>& vertices = pMesh->Vertices;
		const uint32 verticesSize = sizeof(Vertex) * vertices.GetSize();

		byte* pMapping = reinterpret_cast<byte*>(m_VertexStagingBuffer->Map());
		memcpy(pMapping, vertices.GetData(), verticesSize);
		m_VertexStagingBuffer->Unmap();
		pCommandList->CopyBuffer(m_VertexStagingBuffer.Get(), 0, m_VertexBuffer.Get(), 0, verticesSize);
	
		// Copy indices
		const TArray<MeshIndexType>& indices = pMesh->Indices;
		m_IndicesCount = indices.GetSize();
		const uint32 indicesSize = sizeof(MeshIndexType) * m_IndicesCount;

		pMapping = reinterpret_cast<byte*>(m_IndexStagingBuffer->Map());
		memcpy(pMapping, indices.GetData(), indicesSize);
		m_IndexStagingBuffer->Unmap();
		pCommandList->CopyBuffer(m_IndexStagingBuffer.Get(), 0, m_IndexBuffer.Get(), 0, indicesSize);

		// Copy Per FrameBuffer
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		float32 windowWidth = float32(window->GetWidth());
		float32 windowHeight = float32(window->GetHeight());

		glm::mat4 projection = glm::perspective(glm::pi<float>() * 0.5f, windowWidth / windowHeight, 0.1f, 100.0f);
		glm::mat4 view = glm::lookAt(glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0f), g_DefaultUp);

		// Weapon Transformations
		FrameBuffer fb = {
			.Projection = projection,
			.View = view,
			.PrevProjection = glm::mat4(1.0f),
			.PrevView = glm::mat4(1.0f),
			.ViewInv = glm::mat4(1.0f),
			.ProjectionInv = glm::mat4(1.0f),
			.CameraPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
			.CameraRight = glm::vec4(g_DefaultRight, 1.0f),
			.CameraUp = glm::vec4(g_DefaultUp, 1.0f),
			.Jitter = glm::vec2(0, 0),
			.FrameIndex = 0,
			.RandomSeed = 0,
		};

		pMapping = reinterpret_cast<byte*>(m_FrameCopyBuffer->Map());
		memcpy(pMapping, &fb, sizeof(fb));
		m_FrameCopyBuffer->Unmap();
		pCommandList->CopyBuffer(m_FrameCopyBuffer.Get(), 0, m_FrameBuffer.Get(), 0, sizeof(FrameBuffer));

		return true;
	}

	bool FirstPersonWeaponRenderer::CreatePipelineLayout()
	{
		/* Uniform buffers */
		DescriptorBindingDesc frameBufferDesc = {};
		frameBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		frameBufferDesc.DescriptorCount = 1;
		frameBufferDesc.Binding = 0;
		frameBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

		DescriptorBindingDesc weaponDataDesc = {};
		weaponDataDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		weaponDataDesc.DescriptorCount = 1;
		weaponDataDesc.Binding = 4;
		weaponDataDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		/* Storage/Unordered access buffers */
		DescriptorBindingDesc verticesBindingDesc = {};
		verticesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		verticesBindingDesc.DescriptorCount = 1;
		verticesBindingDesc.Binding = 0;
		verticesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

		DescriptorBindingDesc instanceBindingDesc = {};
		instanceBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		instanceBindingDesc.DescriptorCount = 1;
		instanceBindingDesc.Binding = 1;
		instanceBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;
		
		DescriptorBindingDesc meshletBindingDesc = {};
		meshletBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		meshletBindingDesc.DescriptorCount = 1;
		meshletBindingDesc.Binding = 2;
		meshletBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

		DescriptorBindingDesc uniqueIndicesDesc = {};
		uniqueIndicesDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		uniqueIndicesDesc.DescriptorCount = 1;
		uniqueIndicesDesc.Binding = 3;
		uniqueIndicesDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

		DescriptorBindingDesc primitiveIndicesDesc = {};
		primitiveIndicesDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		primitiveIndicesDesc.DescriptorCount = 1;
		primitiveIndicesDesc.Binding = 4;
		primitiveIndicesDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

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

		// LightBuffer
		DescriptorBindingDesc lightBufferDesc = {};
		lightBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		lightBufferDesc.DescriptorCount = 6000;
		lightBufferDesc.Binding = 3;
		lightBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		lightBufferDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

		// u_DepthStencil
		DescriptorBindingDesc depthStencilDesc = {};
		depthStencilDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		depthStencilDesc.DescriptorCount = 6000;
		depthStencilDesc.Binding = 3;
		depthStencilDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		depthStencilDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

		// maps to SET = 0 (BUFFER_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { frameBufferDesc, materialParametersBufferDesc, paintMaskColorsBufferDesc, lightBufferDesc, weaponDataDesc };

		// maps to SET = 1 (TEXTURE_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { albedoMapsDesc, normalMapsDesc, combinedMaterialMapsDesc, depthStencilDesc };

		// maps to SET = 2 (DRAW_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc, meshletBindingDesc, uniqueIndicesDesc, primitiveIndicesDesc };

		// maps to SET = 3 (DRAW_EXTENSION_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc3 = {};
		descriptorSetLayoutDesc3.DescriptorBindings = { paintMaskDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "FirstPersonWeapon Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2, descriptorSetLayoutDesc3 };
		pipelineLayoutDesc.ConstantRanges = { };

		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool FirstPersonWeaponRenderer::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 4;
		
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 9;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "FirstPersonWeapon Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 512;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
	}

	bool FirstPersonWeaponRenderer::CreateShaders()
	{
		m_VertexShaderPointGUID = ResourceManager::LoadShaderFromFile("/FirstPerson/Weapon.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderPointGUID = ResourceManager::LoadShaderFromFile("/FirstPerson/Weapon.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderPointGUID != GUID_NONE && m_PixelShaderPointGUID != GUID_NONE;
	}

	bool FirstPersonWeaponRenderer::CreateCommandLists()
	{
		m_ppGraphicCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppGraphicCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppGraphicCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("FirstPersonWeapon Renderer Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppGraphicCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "FirstPersonWeapon Renderer Graphics Command List " + std::to_string(b);
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

	bool FirstPersonWeaponRenderer::CreateRenderPass(RenderPassAttachmentDesc* pColorAttachmentDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format = pColorAttachmentDesc->Format;
		colorAttachmentDesc.SampleCount = 1;
		colorAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState = pColorAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState = pColorAttachmentDesc->FinalState;

		RenderPassAttachmentDesc depthAttachmentDesc = {};
		depthAttachmentDesc.Format = EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthAttachmentDesc.SampleCount = 1;
		depthAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_CLEAR;
		depthAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		depthAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		depthAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		depthAttachmentDesc.InitialState = ETextureState::TEXTURE_STATE_DONT_CARE;
		depthAttachmentDesc.FinalState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

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
		renderPassDesc.DebugName = "FirstPersonWeapon Renderer Render Pass";
		renderPassDesc.Attachments = { colorAttachmentDesc, depthAttachmentDesc };
		renderPassDesc.Subpasses = { subpassDesc };
		renderPassDesc.SubpassDependencies = { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool FirstPersonWeaponRenderer::CreatePipelineState()
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName = "FirstPersonWeapon Renderer Pipeline Back Cull State";
		pipelineStateDesc.RenderPass = m_RenderPass;
		pipelineStateDesc.PipelineLayout = m_PipelineLayout;

		pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
		pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_BACK;

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

		m_PipelineStateIDBackCull = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);


		ManagedGraphicsPipelineStateDesc pipelineStateDesc2 = {};
		pipelineStateDesc2.DebugName = "FirstPersonWeapon Renderer Pipeline Front Cull State";
		pipelineStateDesc2.RenderPass = m_RenderPass;
		pipelineStateDesc2.PipelineLayout = m_PipelineLayout;

		pipelineStateDesc2.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		pipelineStateDesc2.RasterizerState.LineWidth = 1.f;
		pipelineStateDesc2.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
		pipelineStateDesc2.RasterizerState.CullMode = ECullMode::CULL_MODE_FRONT;

		pipelineStateDesc2.DepthStencilState = {};
		pipelineStateDesc2.DepthStencilState.DepthTestEnable = true;
		pipelineStateDesc2.DepthStencilState.DepthWriteEnable = true;

		pipelineStateDesc2.BlendState.BlendAttachmentStates =
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

		pipelineStateDesc2.VertexShader.ShaderGUID = m_VertexShaderPointGUID;
		pipelineStateDesc2.PixelShader.ShaderGUID = m_PixelShaderPointGUID;

		m_PipelineStateIDFrontCull = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc2);

		return true;

	}
}
