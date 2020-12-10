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

#include "Game/ECS/Components/Misc/InheritanceComponent.h"
#include "Game/ECS/Components/Team/TeamComponent.h"

namespace LambdaEngine
{
	FirstPersonWeaponRenderer::SPushConstantData FirstPersonWeaponRenderer::s_LiquidPushConstantData;

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

		if (!CreateBuffers())
		{
			LOG_ERROR("Failed to create buffers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("Failed to create PipelineLayout");
			return false;
		}

		if (!CreatePipelineLayoutLiquid())
		{
			LOG_ERROR("Failed to create Liquid PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("Failed to create Shaders");
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
				LOG_ERROR("Failed to create resources");
				return false;
			}

			if (!CreateCommandLists())
			{
				LOG_ERROR("Failed to create render command lists");
				return false;
			}

			if (!CreateRenderPass(pPreInitDesc->pColorAttachmentDesc))
			{
				LOG_ERROR("Failed to create RenderPass");
				return false;
			}

			if (!CreatePipelineState())
			{
				LOG_ERROR("Failed to create PipelineState");
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
			succeded = succeded && m_FrameCopyBuffer != nullptr;

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon Renderer Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_FrameBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = succeded && m_FrameBuffer != nullptr;
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
				succeded = succeded && m_WeaponStagingBuffers[b] != nullptr;
			}

			BufferDesc bufferDesc = {};
			bufferDesc.DebugName = "FirstPersonWeapon WeaponData Data Buffer";
			bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
			bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			bufferDesc.SizeInBytes = stagingBufferDesc.SizeInBytes;

			m_WeaponBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			succeded = succeded && m_WeaponBuffer != nullptr;
		}

		return succeded;
	}

	void FirstPersonWeaponRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		using namespace LambdaEngine;

		UNREFERENCED_VARIABLE(backBufferIndex);
		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);

		// Fetch data from the player
		{
			ECSCore* pECSCore = ECSCore::GetInstance();
			if (m_PlayerEntity != UINT32_MAX)
			{
				// Fetch the team index, which is used for coloring the liquid to the same color as the paint.
				const TeamComponent& teamComponent = pECSCore->GetConstComponent<TeamComponent>(m_PlayerEntity);
				s_LiquidPushConstantData.TeamIndex = teamComponent.TeamIndex;

				float dt = (float)delta.AsSeconds();

				// Waves, Code from: https://www.patreon.com/posts/quick-game-art-18245226
				static float s_Time = 0.f;
				s_Time += dt;

				static float s_Recovery = 1.f;
				static float s_WaveAddX = 0.f;
				static float s_WaveAddZ = 0.f;
				static float s_MaxWave = 0.003f;

				// Soften the wave over time.
				s_WaveAddX = glm::mix(s_WaveAddX, 0.f, dt * s_Recovery);
				s_WaveAddZ = glm::mix(s_WaveAddZ, 0.f, dt * s_Recovery);

				float pulse = dt * 2.f * glm::pi<float>();
				s_LiquidPushConstantData.WaveX = s_WaveAddX * glm::sin(pulse * s_Time);
				s_LiquidPushConstantData.WaveZ = s_WaveAddZ * glm::sin(pulse * s_Time);

				// Fetch the player position and rotation to be able to calculate its velocity and angular velocity.
				static glm::vec3 s_PreviousPosition = glm::vec3(0.f);
				static glm::vec3 s_PreviousRotation = glm::vec3(0.f);
				const PositionComponent& positionComponent = pECSCore->GetConstComponent<PositionComponent>(m_PlayerEntity);
				const RotationComponent& rotationComponent = pECSCore->GetConstComponent<RotationComponent>(m_PlayerEntity);
				
				glm::vec3 velocity = (s_PreviousPosition - positionComponent.Position) / dt;

				glm::vec3 eulerAngles = glm::eulerAngles(rotationComponent.Quaternion);
				glm::vec3 angularVelocity = eulerAngles - s_PreviousRotation;

				s_WaveAddX += glm::clamp((velocity.x + (angularVelocity.z * 0.2f)) * s_MaxWave, -s_MaxWave, s_MaxWave);
				s_WaveAddZ += glm::clamp((velocity.z + (angularVelocity.x * 0.2f)) * s_MaxWave, -s_MaxWave, s_MaxWave);

				s_PreviousPosition = positionComponent.Position;
				s_PreviousRotation = eulerAngles;
			}
		}


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

		// Update ammo
		if(m_WeaponEntity != UINT32_MAX)
		{
			WeaponComponent weaponComponent = {};
			bool succeded = ECSCore::GetInstance()->GetConstComponentIf<WeaponComponent>(m_WeaponEntity, weaponComponent);
			if (succeded)
			{
				auto itWater = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_WATER);
				if (itWater != weaponComponent.WeaponTypeAmmo.end())
				{
					s_LiquidPushConstantData.WaterLevel = (float)itWater->second.first / (float)itWater->second.second;
				}

				auto itPaint = weaponComponent.WeaponTypeAmmo.find(EAmmoType::AMMO_TYPE_PAINT);
				if (itPaint != weaponComponent.WeaponTypeAmmo.end())
				{
					s_LiquidPushConstantData.PaintLevel = (float)itPaint->second.first / (float)itPaint->second.second;
				}
			}
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
		else if (resourceName == "GLOBAL_SPECULAR_PROBE")
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 4;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] GLOBAL_SPECULAR_PROBE", setIndex);
			}
		}
		else if (resourceName == "GLOBAL_DIFFUSE_PROBE")
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 5;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] GLOBAL_DIFFUSE_PROBE", setIndex);
			}
		}
		else if (resourceName == "INTEGRATION_LUT")
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_DescriptorSet1 = m_DescriptorCache.GetDescriptorSet("FirstPersonWeapon Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_DescriptorSet1 != nullptr)
			{
				Sampler* pSampler = Sampler::GetLinearSampler();
				uint32 bindingIndex = 6;
				m_DescriptorSet1->WriteTextureDescriptors(ppPerImageTextureViews, &pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, bindingIndex, imageCount, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, false);
			}
			else
			{
				LOG_ERROR("[FirstPersonWeaponRenderer]: Failed to update DescriptorSet[%d] INTEGRATION_LUT", setIndex);
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
				const ComponentArray<WeaponLiquidComponent>* pWeaponLiquidComponents = pECSCore->GetComponentArray<WeaponLiquidComponent>();
				const ComponentArray<WeaponArmsComponent>* pArmsComponents = pECSCore->GetComponentArray<WeaponArmsComponent>();
				const ComponentArray<WeaponLocalComponent>* pWeaponLocalComponents = pECSCore->GetComponentArray<WeaponLocalComponent>();
				const ComponentArray<ParentComponent>* pParentComponents = pECSCore->GetComponentArray<ParentComponent>();

				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					m_DescriptorSetList2[d] = MakeSharedRef(pDrawArgs[d].pDescriptorSet);
				
					if (m_DescriptorSetList2[d] != nullptr)
					{
						for (uint32 i = 0; i < m_pDrawArgs[d].EntityIDs.GetSize(); i++)
						{
							Entity entity = m_pDrawArgs[d].EntityIDs[i];
							if (pWeaponLocalComponents->HasComponent(entity)) {
								
								if (pWeaponLiquidComponents->HasComponent(entity))
								{
									const WeaponLiquidComponent& weaponLiquidComponent = pWeaponLiquidComponents->GetConstData(entity);
									if (weaponLiquidComponent.isWater)
									{
										m_LiquidWaterDrawArgsDescriptorSet = m_pDrawArgs[d].pDescriptorSet;
										m_LiquidWaterEntity = entity;
										m_LiquidWaterIndex = d;

										m_WeaponEntity = pWeaponLocalComponents->GetConstData(entity).weaponEntity;
									}
									else
									{
										m_LiquidPaintDrawArgsDescriptorSet = m_pDrawArgs[d].pDescriptorSet;
										m_LiquidPaintEntity = entity;
										m_LiquidPaintIndex = d;
									}
								}
								else
								{
									if (pArmsComponents->HasComponent(entity))
									{
										if (pParentComponents->HasComponent(entity))
										{
											m_PlayerEntity = pParentComponents->GetConstData(entity).Parent;
										}
										m_ArmsIndex = d;
									}
									else
									{
										m_WeaponIndex = d;
									}

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
			}
			else
			{
				m_DrawCount = 0;
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
				RenderCull(false, m_ArmsIndex, pCommandList, m_PipelineStateIDBackCull);
				RenderLiquid(true, pCommandList); // Render water
				RenderLiquid(false, pCommandList); // Render paint

				RenderCull(true, m_WeaponIndex, pCommandList, m_PipelineStateIDFrontCull);
				RenderCull(true, m_WeaponIndex, pCommandList, m_PipelineStateIDBackCull);
			}
		}

		pCommandList->EndRenderPass();
		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}

	void FirstPersonWeaponRenderer::SetWaterLevel(float waterLevel)
	{
		s_LiquidPushConstantData.WaterLevel = waterLevel;
	}

	void FirstPersonWeaponRenderer::SetPaintLevel(float waterLevel)
	{
		s_LiquidPushConstantData.PaintLevel = waterLevel;
	}

	void FirstPersonWeaponRenderer::RenderCull(bool applyDefaultTransform, uint32 drawArgIndex, CommandList* pCommandList, uint64& pipelineId)
	{
		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(pipelineId));
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet0.Get(), m_PipelineLayout.Get(), 0); // BUFFER_SET_INDEX
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet1.Get(), m_PipelineLayout.Get(), 1); // TEXTURE_SET_INDEX

		const DrawArg& drawArg = m_pDrawArgs[drawArgIndex];
		Entity entity = drawArg.EntityIDs[0];
		glm::mat4 deafultTransform = applyDefaultTransform ? ECSCore::GetInstance()->GetConstComponent<WeaponLocalComponent>(entity).DefaultTransform : glm::mat4(1.f);
		pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, (void*)&deafultTransform, sizeof(glm::mat4), 0);

		// Draw Weapon
		pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSetList2[drawArgIndex].Get(), m_PipelineLayout.Get(), 2); // Mesh data (Vertices and instance buffers)
		pCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
	}

	void FirstPersonWeaponRenderer::RenderLiquid(bool isWater, CommandList* pCommandList)
	{
		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateIDNoCull));
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet0.Get(), m_LiquidPipelineLayout.Get(), 0); // BUFFER_SET_INDEX
		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet1.Get(), m_LiquidPipelineLayout.Get(), 1); // TEXTURE_SET_INDEX
		
		const DrawArg& drawArg = m_pDrawArgs[isWater ? m_LiquidWaterIndex : m_LiquidPaintIndex];
		Entity entity = drawArg.EntityIDs[0];
		WeaponLocalComponent localWeaponComponent = ECSCore::GetInstance()->GetConstComponent<WeaponLocalComponent>(entity);
		s_LiquidPushConstantData.DefaultTransform = localWeaponComponent.DefaultTransform;
		s_LiquidPushConstantData.IsWater = isWater ? 1.f : 0.f;

		pCommandList->SetConstantRange(m_LiquidPipelineLayout.Get(), 
			FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, 
			(void*)&s_LiquidPushConstantData, sizeof(SPushConstantData), 0);

		// Draw Weapon liquid
		pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);
		pCommandList->BindDescriptorSetGraphics(isWater ? m_LiquidWaterDrawArgsDescriptorSet : m_LiquidPaintDrawArgsDescriptorSet, m_LiquidPipelineLayout.Get(), 2); // Draw arg data
		pCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
	}

	void FirstPersonWeaponRenderer::UpdateWeaponBuffer(CommandList* pCommandList, uint32 modFrameIndex)
	{
		const ComponentArray<PositionComponent>* pPositionComponents = ECSCore::GetInstance()->GetComponentArray<PositionComponent>();
		const ComponentArray<RotationComponent>* pRotationComponents = ECSCore::GetInstance()->GetComponentArray<RotationComponent>();
		
		if (m_PlayerEntity != MAXUINT32 && pPositionComponents->HasComponent(m_PlayerEntity)) {
			SWeaponBuffer data = {};

			data.Model = glm::translate(glm::vec3(0.0f, -0.375f, 0.1f));
			data.Model = glm::scale(data.Model, glm::vec3(1.0f, 1.0f, 1.0f));
			data.PlayerPos = pPositionComponents->GetConstData(m_PlayerEntity).Position;
			data.PlayerRotaion = glm::toMat4(pRotationComponents->GetConstData(m_PlayerEntity).Quaternion);

			Buffer* pStagingBuffer = m_WeaponStagingBuffers[modFrameIndex].Get();
			byte* pMapping = reinterpret_cast<byte*>(pStagingBuffer->Map());
			memcpy(pMapping, &data, sizeof(data));
			pStagingBuffer->Unmap();
			pCommandList->CopyBuffer(pStagingBuffer, 0, m_WeaponBuffer.Get(), 0, sizeof(SWeaponBuffer));
		}
	}

	bool FirstPersonWeaponRenderer::PrepareResources(CommandList* pCommandList)
	{
		// Copy Per FrameBuffer
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		float32 windowWidth = float32(window->GetWidth());
		float32 windowHeight = float32(window->GetHeight());

		glm::mat4 projection = glm::perspective(glm::pi<float>() * 0.5f, windowWidth / windowHeight, 0.001f, 10.0f);
		glm::mat4 view =  glm::lookAt(glm::vec3(0.0, 0.0, 0.1), glm::vec3(0.0f), g_DefaultUp);

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

		byte* pMapping = reinterpret_cast<byte*>(m_FrameCopyBuffer->Map());
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

		// u_GlobalSpecularProbe
		DescriptorBindingDesc globalSpecularProbeDesc = {};
		globalSpecularProbeDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		globalSpecularProbeDesc.DescriptorCount = 1;
		globalSpecularProbeDesc.Binding = 4;
		globalSpecularProbeDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// u_GlobalDiffuseProbe
		DescriptorBindingDesc globalDiffuseProbeDesc = {};
		globalDiffuseProbeDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		globalDiffuseProbeDesc.DescriptorCount = 1;
		globalDiffuseProbeDesc.Binding = 5;
		globalDiffuseProbeDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// u_IntegrationLUT
		DescriptorBindingDesc integrationLUTDesc = {};
		integrationLUTDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		integrationLUTDesc.DescriptorCount = 1;
		integrationLUTDesc.Binding = 6;
		integrationLUTDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// maps to SET = 0 (BUFFER_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { frameBufferDesc, materialParametersBufferDesc, paintMaskColorsBufferDesc, lightBufferDesc, weaponDataDesc };

		// maps to SET = 1 (TEXTURE_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { albedoMapsDesc, normalMapsDesc, combinedMaterialMapsDesc, depthStencilDesc, 
			globalSpecularProbeDesc, globalDiffuseProbeDesc, integrationLUTDesc };

		// maps to SET = 2 (DRAW_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc, meshletBindingDesc, uniqueIndicesDesc, primitiveIndicesDesc };

		ConstantRangeDesc constantRangeDesc = {};
		constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		constantRangeDesc.OffsetInBytes = 0;
		constantRangeDesc.SizeInBytes = sizeof(glm::mat4);

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "FirstPersonWeapon Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };
		pipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool FirstPersonWeaponRenderer::CreatePipelineLayoutLiquid()
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

		// u_GlobalSpecularProbe
		DescriptorBindingDesc globalSpecularProbeDesc = {};
		globalSpecularProbeDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		globalSpecularProbeDesc.DescriptorCount = 1;
		globalSpecularProbeDesc.Binding = 4;
		globalSpecularProbeDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// u_GlobalDiffuseProbe
		DescriptorBindingDesc globalDiffuseProbeDesc = {};
		globalDiffuseProbeDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		globalDiffuseProbeDesc.DescriptorCount = 1;
		globalDiffuseProbeDesc.Binding = 5;
		globalDiffuseProbeDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// u_IntegrationLUT
		DescriptorBindingDesc integrationLUTDesc = {};
		integrationLUTDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		integrationLUTDesc.DescriptorCount = 1;
		integrationLUTDesc.Binding = 6;
		integrationLUTDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// maps to SET = 0 (BUFFER_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { frameBufferDesc, materialParametersBufferDesc, paintMaskColorsBufferDesc, lightBufferDesc, weaponDataDesc };

		// maps to SET = 1 (TEXTURE_SET_INDEX)
		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { albedoMapsDesc, normalMapsDesc, combinedMaterialMapsDesc, depthStencilDesc,
			globalSpecularProbeDesc, globalDiffuseProbeDesc, integrationLUTDesc };

		// Set 2
		DescriptorSetLayoutDesc drawArgDescriptorSetLayoutDesc = {};
		{
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

			DescriptorBindingDesc meshletsBindingDesc = {};
			meshletsBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			meshletsBindingDesc.DescriptorCount = 1;
			meshletsBindingDesc.Binding = 2;
			meshletsBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			DescriptorBindingDesc uniqueIndicesBindingDesc = {};
			uniqueIndicesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			uniqueIndicesBindingDesc.DescriptorCount = 1;
			uniqueIndicesBindingDesc.Binding = 3;
			uniqueIndicesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			DescriptorBindingDesc primitiveIndicesBindingDesc = {};
			primitiveIndicesBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
			primitiveIndicesBindingDesc.DescriptorCount = 1;
			primitiveIndicesBindingDesc.Binding = 4;
			primitiveIndicesBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_ALL;

			TArray<DescriptorBindingDesc> descriptorBindings = {
				verticesBindingDesc,
				instanceBindingDesc,
				meshletsBindingDesc,
				uniqueIndicesBindingDesc,
				primitiveIndicesBindingDesc
			};
			
			drawArgDescriptorSetLayoutDesc.DescriptorBindings = descriptorBindings;
		}

		ConstantRangeDesc constantRangeDesc = {};
		constantRangeDesc.ShaderStageFlags = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		constantRangeDesc.OffsetInBytes = 0;
		constantRangeDesc.SizeInBytes = sizeof(SPushConstantData);

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "FirstPersonWeapon Renderer Pipeline Layout liquid";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, drawArgDescriptorSetLayoutDesc };
		pipelineLayoutDesc.ConstantRanges = { constantRangeDesc };

		m_LiquidPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_LiquidPipelineLayout != nullptr;
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

		m_LiquidVertexShaderGUID = ResourceManager::LoadShaderFromFile("/FirstPerson/WeaponLiquid.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_LiquidPixelShaderGUID = ResourceManager::LoadShaderFromFile("/FirstPerson/WeaponLiquid.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderPointGUID != GUID_NONE && m_PixelShaderPointGUID != GUID_NONE && m_LiquidVertexShaderGUID != GUID_NONE && m_LiquidPixelShaderGUID != GUID_NONE;
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
		// Back face cullling pipeline
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
		}

		// Front face cullling pipeline
		{
			ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
			pipelineStateDesc.DebugName = "FirstPersonWeapon Renderer Pipeline Front Cull State";
			pipelineStateDesc.RenderPass = m_RenderPass;
			pipelineStateDesc.PipelineLayout = m_PipelineLayout;

			pipelineStateDesc.InputAssembly.PrimitiveTopology = EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

			pipelineStateDesc.RasterizerState.LineWidth = 1.f;
			pipelineStateDesc.RasterizerState.PolygonMode = EPolygonMode::POLYGON_MODE_FILL;
			pipelineStateDesc.RasterizerState.CullMode = ECullMode::CULL_MODE_FRONT;

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

			m_PipelineStateIDFrontCull = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		}

		// No cullling pipeline
		{
			ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
			pipelineStateDesc.DebugName = "FirstPersonWeapon Renderer Pipeline No Cull State";
			pipelineStateDesc.RenderPass = m_RenderPass;
			pipelineStateDesc.PipelineLayout = m_LiquidPipelineLayout;

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

			pipelineStateDesc.VertexShader.ShaderGUID = m_LiquidVertexShaderGUID;
			pipelineStateDesc.PixelShader.ShaderGUID = m_LiquidPixelShaderGUID;

			m_PipelineStateIDNoCull = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		}

		return true;

	}
}
