#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/SwapChain.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/AccelerationStructure.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/RenderGraph.h"
#include "Rendering/RenderGraphSerializer.h"
#include "Rendering/ImGuiRenderer.h"
#include "Rendering/EntityMaskManager.h"
#include "Rendering/LineRenderer.h"
#include "Rendering/PaintMaskRenderer.h"
#include "Rendering/StagingBufferCache.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"
#include "Game/ECS/Components/Rendering/ParticleEmitter.h"
#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"
#include "Game/ECS/Components/Rendering/RayTracedComponent.h"
#include "Game/ECS/Components/Player/PlayerComponent.h"

#include "Rendering/ParticleRenderer.h"
#include "Rendering/ParticleUpdater.h"
#include "Rendering/RT/ASBuilder.h"

#include "GUI/Core/GUIApplication.h"
#include "GUI/Core/GUIRenderer.h"

#include "Engine/EngineConfig.h"
#include "Game/Multiplayer/MultiplayerUtils.h"

namespace LambdaEngine
{
	RenderSystem RenderSystem::s_Instance;

	bool RenderSystem::Init()
	{
		GraphicsDeviceFeatureDesc deviceFeatures;
		RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
		m_RayTracingEnabled		= deviceFeatures.RayTracing && EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_RAY_TRACING);
		m_MeshShadersEnabled	= deviceFeatures.MeshShaders && EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_MESH_SHADER);

		// Subscribe on Static Entities & Dynamic Entities
		{
			TransformGroup transformGroup;
			transformGroup.Position.Permissions	= R;
			transformGroup.Scale.Permissions	= R;
			transformGroup.Rotation.Permissions	= R;

			SystemRegistration systemReg = {};
			systemReg.Phase = LAST_PHASE;
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					.pSubscriber = &m_StaticMeshEntities,
					.ComponentAccesses =
					{
						{ R, MeshComponent::Type() }
					},
					.ComponentGroups =
					{
						&transformGroup
					},
					.ExcludedComponentTypes =
					{
						PlayerBaseComponent::Type(),
						AnimationComponent::Type(),
						AnimationAttachedComponent::Type(),
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnStaticMeshEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::RemoveRenderableEntity, this)
				},
				{
					.pSubscriber = &m_AnimatedEntities,
					.ComponentAccesses =
					{
						{ R, AnimationComponent::Type() },
						{ R, MeshComponent::Type() }
					},
					.ComponentGroups =
					{
						&transformGroup
					},
					.ExcludedComponentTypes =
					{
						PlayerBaseComponent::Type(),
						AnimationAttachedComponent::Type(),
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnAnimatedEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::RemoveRenderableEntity, this)
				},
				{
					.pSubscriber = &m_AnimationAttachedEntities,
					.ComponentAccesses =
					{
						{ R, AnimationAttachedComponent::Type() },
						{ R, MeshComponent::Type() }
					},
					.ComponentGroups =
					{
						&transformGroup
					},
					.ExcludedComponentTypes =
					{
						PlayerBaseComponent::Type(),
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnAnimationAttachedEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::RemoveRenderableEntity, this)
				},
				{
					.pSubscriber = &m_LocalPlayerEntities,
					.ComponentAccesses =
					{
						{ NDA, PlayerBaseComponent::Type() },
						{ R, AnimationComponent::Type() },
						{ R, MeshComponent::Type() }
					},
					.ComponentGroups =
					{
						&transformGroup
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnPlayerEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::RemoveRenderableEntity, this)
				},
				{
					.pSubscriber = &m_DirectionalLightEntities,
					.ComponentAccesses =
					{
						{ R, DirectionalLightComponent::Type() },
						{ R, PositionComponent::Type() },
						{ R, RotationComponent::Type() }
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnDirectionalEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::OnDirectionalEntityRemoved, this)
				},
				{
					.pSubscriber = &m_PointLightEntities,
					.ComponentAccesses =
					{
						{ R, PointLightComponent::Type() },
						{ R, PositionComponent::Type() }
					},
					.OnEntityAdded = std::bind_front(&RenderSystem::OnPointLightEntityAdded, this),
					.OnEntityRemoval = std::bind_front(&RenderSystem::OnPointLightEntityRemoved, this)
				},
				{
					.pSubscriber = &m_CameraEntities,
					.ComponentAccesses =
					{
						{ R, ViewProjectionMatricesComponent::Type() },
						{ R, CameraComponent::Type() }
					},
					.ComponentGroups =
					{
						&transformGroup
					}
				},
				{
					.pSubscriber = &m_ParticleEmitters,
					.ComponentAccesses =
					{
						{ RW, ParticleEmitterComponent::Type() },
						{ RW, PositionComponent::Type() },
						{ RW, RotationComponent::Type() }
					},
					.OnEntityRemoval = std::bind(&RenderSystem::OnEmitterEntityRemoved, this, std::placeholders::_1)
				}
			};

			systemReg.SubscriberRegistration.AdditionalAccesses =
			{
				{ R, MeshPaintComponent::Type() }
			};

			RegisterSystem(TYPE_NAME(RenderSystem), systemReg);
		}

		Window* pActiveWindow = CommonApplication::Get()->GetActiveWindow().Get();
		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName = "Renderer Swap Chain";
			swapChainDesc.pWindow = pActiveWindow;
			swapChainDesc.pQueue = RenderAPI::GetGraphicsQueue();
			swapChainDesc.Format = EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width = pActiveWindow->GetWidth();
			swapChainDesc.Height = pActiveWindow->GetHeight();
			swapChainDesc.BufferCount = BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount = 1;
			swapChainDesc.VerticalSync = false;

			m_SwapChain = RenderAPI::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!m_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			m_ppBackBuffers = DBG_NEW Texture * [BACK_BUFFER_COUNT];
			m_ppBackBufferViews = DBG_NEW TextureView * [BACK_BUFFER_COUNT];

			m_FrameIndex++;
			m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		// Per Frame Buffer
		{
			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				BufferDesc perFrameCopyBufferDesc = {};
				perFrameCopyBufferDesc.DebugName		= "Scene Per Frame Staging Buffer " + std::to_string(b);
				perFrameCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				perFrameCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				perFrameCopyBufferDesc.SizeInBytes		= sizeof(PerFrameBuffer);

				m_ppPerFrameStagingBuffers[b] = RenderAPI::GetDevice()->CreateBuffer(&perFrameCopyBufferDesc);
			}

			BufferDesc perFrameBufferDesc = {};
			perFrameBufferDesc.DebugName			= "Scene Per Frame Buffer";
			perFrameBufferDesc.MemoryType			= EMemoryType::MEMORY_TYPE_GPU;
			perFrameBufferDesc.Flags				= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
			perFrameBufferDesc.SizeInBytes			= sizeof(PerFrameBuffer);

			m_pPerFrameBuffer = RenderAPI::GetDevice()->CreateBuffer(&perFrameBufferDesc);
		} 

		// Create animation resources
		{
			DescriptorHeapDesc descriptorHeap;
			descriptorHeap.DebugName											= "Animation DescriptorHeao";
			descriptorHeap.DescriptorSetCount									= 1024;
			descriptorHeap.DescriptorCount.UnorderedAccessBufferDescriptorCount	= 4;

			m_AnimationDescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeap);
			if (!m_AnimationDescriptorHeap)
			{
				return false;
			}

			DescriptorSetLayoutDesc descriptorSetLayoutDesc;
			descriptorSetLayoutDesc.DescriptorSetLayoutFlags = 0;
			descriptorSetLayoutDesc.DescriptorBindings =
			{
				{ EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER, 1, 0, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER },
				{ EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER, 1, 1, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER },
				{ EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER, 1, 2, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER },
				{ EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER, 1, 3, FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER }
			};

			PipelineLayoutDesc pipelineLayoutDesc;
			pipelineLayoutDesc.DebugName			= "Skinning pipeline";
			pipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc };
			pipelineLayoutDesc.ConstantRanges		=
			{
				{
					FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER,
					4,
					0
				}
			};

			m_SkinningPipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);
			if (!m_SkinningPipelineLayout)
			{
				return false;
			}

			ManagedComputePipelineStateDesc pipelineDesc;
			pipelineDesc.DebugName			= "Skinning pipeline";
			pipelineDesc.PipelineLayout		= m_SkinningPipelineLayout;
			pipelineDesc.Shader.ShaderGUID	= ResourceManager::LoadShaderFromFile("Animation/Skinning.comp", FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, EShaderLang::SHADER_LANG_GLSL);

			m_SkinningPipelineID = PipelineStateManager::CreateComputePipelineState(&pipelineDesc);
			if (m_SkinningPipelineID == 0)
			{
				return false;
			}

			// Set colors for the teams
			{
				// First is default and should always be 0
				m_PaintMaskColors.PushBack({ 0.0, 0.0, 0.0, 1.0 });

				// Team 1 (default red)
				m_PaintMaskColors.PushBack({ 214.f/255.f, 0.f/255.f, 0.f/255.f, 1.0 });

				// Team 2 (default blue)
				m_PaintMaskColors.PushBack({ 0.f/255.f, 29.f/255.f, 214.f/255.f, 1.0 });

				m_PaintMaskColorsResourceDirty = true;
			}
		}

		return true;
	}


	bool RenderSystem::InitRenderGraphs() {

		Window* pActiveWindow = CommonApplication::Get()->GetActiveWindow().Get();

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			String renderGraphName = EngineConfig::GetStringProperty(EConfigOption::CONFIG_OPTION_RENDER_GRAPH_NAME);
			if (renderGraphName != "")
			{
				String prefix = m_RayTracingEnabled ? "RT_" : "";
				String postfix = m_MeshShadersEnabled ? "_MESH" : "";
				size_t pos = renderGraphName.find_first_of(".lrg");
				if (pos != String::npos)
				{
					renderGraphName.insert(pos, postfix);
				}
				else
				{
					renderGraphName += postfix + ".lrg";
				}

				renderGraphName = prefix + renderGraphName;
			}

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, renderGraphName, IMGUI_ENABLED, EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_LINE_RENDERER)))
			{
				LOG_ERROR("[RenderSystem]: Failed to Load RenderGraph, loading Default...");

				renderGraphStructure = {};
				RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "", true, EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_LINE_RENDERER));
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.Name = "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc = &renderGraphStructure;
			renderGraphDesc.BackBufferCount = BACK_BUFFER_COUNT;
			renderGraphDesc.BackBufferWidth = pActiveWindow->GetWidth();
			renderGraphDesc.BackBufferHeight = pActiveWindow->GetHeight();
			renderGraphDesc.CustomRenderers = { };

			// Add paint mask renderer to the custom renderers inside the render graph.
			{
				m_pPaintMaskRenderer = DBG_NEW PaintMaskRenderer(RenderAPI::GetDevice(), BACK_BUFFER_COUNT);
				m_pPaintMaskRenderer->Init();

				renderGraphDesc.CustomRenderers.PushBack(m_pPaintMaskRenderer);
			}

			// Light Renderer
			bool isServer = MultiplayerUtils::IsServer();
			if (!isServer)
			{
				m_pLightRenderer = DBG_NEW LightRenderer();
				m_pLightRenderer->Init();

				renderGraphDesc.CustomRenderers.PushBack(m_pLightRenderer);
			}

			// AS Builder
			if (m_RayTracingEnabled)
			{
				m_pASBuilder = DBG_NEW ASBuilder();
				m_pASBuilder->Init();

				renderGraphDesc.CustomRenderers.PushBack(m_pASBuilder);
			}

			// Particle Renderer & Manager
			if (!isServer)
			{
				constexpr uint32 MAX_PARTICLE_COUNT = 20000U;
				m_ParticleManager.Init(MAX_PARTICLE_COUNT, m_pASBuilder);

				m_pParticleRenderer = DBG_NEW ParticleRenderer();
				m_pParticleRenderer->Init();
				renderGraphDesc.CustomRenderers.PushBack(m_pParticleRenderer);

				m_pParticleUpdater = DBG_NEW ParticleUpdater();
				m_pParticleUpdater->Init();
				renderGraphDesc.CustomRenderers.PushBack(m_pParticleUpdater);
			}

			//GUI Renderer
			{
				CustomRenderer* pGUIRenderer = GUIApplication::GetRenderer();
				renderGraphDesc.CustomRenderers.PushBack(pGUIRenderer);
			}

			// Other Custom Renderers constructed in game
			if (!m_GameSpecificCustomRenderers.IsEmpty())
			{
				for (auto* pCustomRenderer : m_GameSpecificCustomRenderers)
				{
					if (pCustomRenderer)
					{
						pCustomRenderer->Init();
						renderGraphDesc.CustomRenderers.PushBack(pCustomRenderer);
					}
				}
			}

			m_pRenderGraph = DBG_NEW RenderGraph(RenderAPI::GetDevice());
			if (!m_pRenderGraph->Init(&renderGraphDesc, m_RequiredDrawArgs))
			{
				LOG_ERROR("[RenderSystem]: Failed to initialize RenderGraph");
				return false;
			}
		}

		//Update RenderGraph with Back Buffer
		{
			for (uint32 v = 0; v < BACK_BUFFER_COUNT; v++)
			{
				m_ppBackBuffers[v] = m_SwapChain->GetBuffer(v);
				m_ppBackBufferViews[v] = m_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName = RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures = m_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_ppBackBufferViews;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
		} 

		UpdateBuffers();
		UpdateRenderGraph();

		return true;
	}

	bool RenderSystem::Release()
	{
		for (auto& meshAndInstancesIt : m_MeshAndInstancesMap)
		{
			SAFERELEASE(meshAndInstancesIt.second.pPrimitiveIndices);
			SAFERELEASE(meshAndInstancesIt.second.pUniqueIndices);
			SAFERELEASE(meshAndInstancesIt.second.pMeshlets);
			SAFERELEASE(meshAndInstancesIt.second.pVertexBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pAnimatedVertexBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pVertexWeightsBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pAnimationDescriptorSet);
			SAFERELEASE(meshAndInstancesIt.second.pBoneMatrixBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pStagingMatrixBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pIndexBuffer);
			SAFERELEASE(meshAndInstancesIt.second.pRasterInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				SAFERELEASE(meshAndInstancesIt.second.ppRasterInstanceStagingBuffers[b]);
			}
		}

		SAFEDELETE(m_pLineRenderer);
		SAFEDELETE(m_pPaintMaskRenderer);
		SAFEDELETE(m_pLightRenderer);
		SAFEDELETE(m_pParticleRenderer);
		SAFEDELETE(m_pParticleUpdater);
		SAFEDELETE(m_pASBuilder);

		// Delete Custom Renderers
		for (uint32 c = 0; c < m_GameSpecificCustomRenderers.GetSize(); c++)
		{
			SAFEDELETE(m_GameSpecificCustomRenderers[c]);
		}

		// Remove Pointlight Texture and Texture Views
		for (uint32 c = 0; c < m_CubeTextures.GetSize(); c++)
		{
			SAFERELEASE(m_CubeTextures[c]);
			SAFERELEASE(m_CubeTextureViews[c]);
		}

		for (uint32 f = 0; f < m_CubeSubImageTextureViews.GetSize(); f++)
		{
			SAFERELEASE(m_CubeSubImageTextureViews[f]);
		}

		for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
		{
			TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[b];
			for (DeviceChild* pResource : resourcesToRemove)
			{
				SAFERELEASE(pResource);
			}

			resourcesToRemove.Clear();

			SAFERELEASE(m_ppMaterialParametersStagingBuffers[b]);
			SAFERELEASE(m_ppPerFrameStagingBuffers[b]);
			SAFERELEASE(m_ppLightsStagingBuffer[b]);
			SAFERELEASE(m_ppPaintMaskColorStagingBuffers[b]);
		}

		SAFERELEASE(m_pMaterialParametersBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pLightsBuffer);
		SAFERELEASE(m_pPaintMaskColorBuffer);

		m_ParticleManager.Release();

		SAFEDELETE(m_pRenderGraph);

		if (m_SwapChain)
		{
			for (uint32 i = 0; i < BACK_BUFFER_COUNT; i++)
			{
				SAFERELEASE(m_ppBackBuffers[i]);
				SAFERELEASE(m_ppBackBufferViews[i]);
			}

			SAFEDELETE_ARRAY(m_ppBackBuffers);
			SAFEDELETE_ARRAY(m_ppBackBufferViews);
			m_SwapChain.Reset();
		}

		m_AnimationDescriptorHeap.Reset();
		m_SkinningPipelineLayout.Reset();
		return true;
	}

	void RenderSystem::Tick(Timestamp deltaTime)
	{
		UNREFERENCED_VARIABLE(deltaTime);

		ECSCore* pECSCore = ECSCore::GetInstance();

		const ComponentArray<PositionComponent>*	pPositionComponents = pECSCore->GetComponentArray<PositionComponent>();
		const ComponentArray<RotationComponent>*	pRotationComponents = pECSCore->GetComponentArray<RotationComponent>();
		const ComponentArray<ScaleComponent>*		pScaleComponents	= pECSCore->GetComponentArray<ScaleComponent>();

		const ComponentArray<PointLightComponent>* pPointLightComponents = pECSCore->GetComponentArray<PointLightComponent>();
		for (Entity entity : m_PointLightEntities.GetIDs())
		{
			const auto& pointLight 	= pPointLightComponents->GetConstData(entity);
			const auto& position 	= pPositionComponents->GetConstData(entity);
			if (pointLight.Dirty || position.Dirty)
			{
				UpdatePointLight(entity, position.Position, pointLight.ColorIntensity, pointLight.NearPlane, pointLight.FarPlane);
			}
		}

		ComponentArray<DirectionalLightComponent>* pDirLightComponents = pECSCore->GetComponentArray<DirectionalLightComponent>();
		for (Entity entity : m_DirectionalLightEntities.GetIDs())
		{
			const auto& dirLight = pDirLightComponents->GetConstData(entity);
			const auto& position = pPositionComponents->GetConstData(entity);
			const auto& rotation = pRotationComponents->GetConstData(entity);
			if (dirLight.Dirty || rotation.Dirty || position.Dirty)
			{
				UpdateDirectionalLight(
					dirLight.ColorIntensity,
					position.Position,
					rotation.Quaternion,
					dirLight.FrustumWidth,
					dirLight.FrustumHeight,
					dirLight.FrustumZNear,
					dirLight.FrustumZFar
				);
			}
		}

		const ComponentArray<CameraComponent>*					pCameraComponents 	= pECSCore->GetComponentArray<CameraComponent>();
		const ComponentArray<ViewProjectionMatricesComponent>* 	pViewProjComponents	= pECSCore->GetComponentArray<ViewProjectionMatricesComponent>();
		for (Entity entity : m_CameraEntities.GetIDs())
		{
			const auto& cameraComp = pCameraComponents->GetConstData(entity);
			if (cameraComp.IsActive)
			{
				const auto& positionComp = pPositionComponents->GetConstData(entity);
				const auto& rotationComp = pRotationComponents->GetConstData(entity);
				const auto& viewProjComp = pViewProjComponents->GetConstData(entity);
				UpdateCamera(positionComp.Position, rotationComp.Quaternion, cameraComp, viewProjComp);
			}
		}

		ComponentArray<MeshComponent>*				pMeshComponents					= pECSCore->GetComponentArray<MeshComponent>();
		ComponentArray<AnimationComponent>*			pAnimationComponents			= pECSCore->GetComponentArray<AnimationComponent>();
		ComponentArray<AnimationAttachedComponent>*	pAnimationAttachedComponents	= pECSCore->GetComponentArray<AnimationAttachedComponent>();
		{
			for (Entity entity : m_LocalPlayerEntities)
			{
				MeshComponent&		meshComp		= pMeshComponents->GetData(entity);
				AnimationComponent&	animationComp	= pAnimationComponents->GetData(entity);
				const auto&			positionComp	= pPositionComponents->GetConstData(entity);
				const auto&			rotationComp	= pRotationComponents->GetConstData(entity);
				const auto&			scaleComp		= pScaleComponents->GetConstData(entity);

				UpdateAnimation(entity, meshComp, animationComp);
				UpdateTransform(entity, positionComp, rotationComp, scaleComp, glm::bvec3(false, true, false));
			}

			for (Entity entity : m_AnimatedEntities)
			{
				MeshComponent&		meshComp		= pMeshComponents->GetData(entity);
				AnimationComponent&	animationComp	= pAnimationComponents->GetData(entity);
				const auto&			positionComp	= pPositionComponents->GetConstData(entity);
				const auto&			rotationComp	= pRotationComponents->GetConstData(entity);
				const auto&			scaleComp		= pScaleComponents->GetConstData(entity);

				UpdateAnimation(entity, meshComp, animationComp);
				UpdateTransform(entity, positionComp, rotationComp, scaleComp, glm::bvec3(true));
			}

			for (Entity entity : m_AnimationAttachedEntities)
			{
				const auto&	animationAttachedComponent	= pAnimationAttachedComponents->GetConstData(entity);
				const auto&	positionComp				= pPositionComponents->GetConstData(entity);
				const auto&	rotationComp				= pRotationComponents->GetConstData(entity);
				const auto&	scaleComp					= pScaleComponents->GetConstData(entity);

				UpdateTransform(entity, animationAttachedComponent.Transform, positionComp, rotationComp, scaleComp, glm::bvec3(false, true, false));
			}
		}

		for (Entity entity : m_StaticMeshEntities)
		{
			const auto& positionComp	= pPositionComponents->GetConstData(entity);
			const auto& rotationComp	= pRotationComponents->GetConstData(entity);
			const auto& scaleComp		= pScaleComponents->GetConstData(entity);

			UpdateTransform(entity, positionComp, rotationComp, scaleComp, glm::bvec3(true));
		}

		ComponentArray<ParticleEmitterComponent>* pEmitterComponents = pECSCore->GetComponentArray<ParticleEmitterComponent>();
		for (Entity entity : m_ParticleEmitters)
		{
			const auto& positionComp = pPositionComponents->GetConstData(entity);
			const auto& rotationComp = pRotationComponents->GetConstData(entity);
			const auto& emitterComp = pEmitterComponents->GetConstData(entity);

			if (positionComp.Dirty || rotationComp.Dirty || emitterComp.Dirty)
				UpdateParticleEmitter(entity, positionComp, rotationComp, emitterComp);

			// If onetime emitter we want to reset active
			if (emitterComp.OneTime && emitterComp.Active)
			{
				auto& emitterCompNonConst = pEmitterComponents->GetData(entity);
				emitterCompNonConst.Active = false;
			}
		}

		// Tick Particle Manager
		if (m_ParticleManager.IsInitilized())
		{
			m_ParticleManager.Tick(deltaTime, m_ModFrameIndex);

			// Particle Updates
			uint32 particleCount = m_ParticleManager.GetParticleCount();
			uint32 activeEmitterCount = m_ParticleManager.GetActiveEmitterCount();
			m_pParticleRenderer->SetCurrentParticleCount(particleCount, activeEmitterCount);
			m_pParticleUpdater->SetCurrentParticleCount(particleCount, activeEmitterCount);
		}
	}

	bool RenderSystem::Render(Timestamp delta)
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		StagingBufferCache::Tick();
		CleanBuffers();

		UpdateBuffers();
		UpdateRenderGraph();

		m_pRenderGraph->Update(delta, (uint32)m_ModFrameIndex, m_BackBufferIndex);

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		return true;
	}

	void RenderSystem::Flush()
	{
		m_pRenderGraph->Flush();

		/*
		m_DirtyDrawArgs.clear();
		m_PointLightTextureUpdateQueue.Clear();

		m_LightsResourceDirty			= false;
		m_PaintMaskColorsResourceDirty	= false;
		m_MaterialsResourceDirty		= false;
		m_PerFrameResourceDirty			= false;
		*/
	}

	void RenderSystem::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		Window* pActiveWindow = CommonApplication::Get()->GetActiveWindow().Get();

		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name						= name;
		renderGraphDesc.pRenderGraphStructureDesc	= pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
		renderGraphDesc.BackBufferWidth				= pActiveWindow->GetWidth();
		renderGraphDesc.BackBufferHeight			= pActiveWindow->GetHeight();

		if (EngineConfig::GetBoolProperty(EConfigOption::CONFIG_OPTION_LINE_RENDERER))
		{
			m_pLineRenderer = DBG_NEW LineRenderer(RenderAPI::GetDevice(), MEGA_BYTE(1), BACK_BUFFER_COUNT);
			m_pLineRenderer->Init();

			renderGraphDesc.CustomRenderers.PushBack(m_pLineRenderer);
		}

		// Light Renderer
		if (m_RayTracingEnabled)
		{
			renderGraphDesc.CustomRenderers.PushBack(m_pLightRenderer);
		}

		// Particles
		if (m_RayTracingEnabled)
		{
			renderGraphDesc.CustomRenderers.PushBack(m_pParticleRenderer);
			renderGraphDesc.CustomRenderers.PushBack(m_pParticleUpdater);
		}

		// AS Builder
		if (m_RayTracingEnabled)
		{
			renderGraphDesc.CustomRenderers.PushBack(m_pASBuilder);
		}

		// GUI Renderer
		{
			CustomRenderer* pGUIRenderer = GUIApplication::GetRenderer();
			renderGraphDesc.CustomRenderers.PushBack(pGUIRenderer);
		}

		// Paint Mask Renderer
		{
			m_pPaintMaskRenderer = DBG_NEW PaintMaskRenderer(RenderAPI::GetDevice(), BACK_BUFFER_COUNT);
			m_pPaintMaskRenderer->Init();

			renderGraphDesc.CustomRenderers.PushBack(m_pPaintMaskRenderer);
		}

		m_RequiredDrawArgs.clear();
		if (!m_pRenderGraph->Recreate(&renderGraphDesc, m_RequiredDrawArgs))
		{
			LOG_ERROR("[Renderer]: Failed to set new RenderGraph %s", name.c_str());
		}

		m_DirtyDrawArgs						= m_RequiredDrawArgs;
		m_PerFrameResourceDirty				= true;
		m_MaterialsResourceDirty			= true;
		m_MaterialsPropertiesBufferDirty	= true;
		m_LightsBufferDirty					= true;
		m_PointLightsDirty					= true;

		UpdateRenderGraph();
	}

	void RenderSystem::AddCustomRenderer(CustomRenderer* pCustomRenderer)
	{
		if (!pCustomRenderer)
		{
			LOG_WARNING("[RenderSystem]: AddCustomRenderer failed - CustomRenderer not constructed");
			return;
		}
		m_GameSpecificCustomRenderers.PushBack(pCustomRenderer);
	}

	void RenderSystem::SetRenderStageSleeping(const String& renderStageName, bool sleeping)
	{
		if (m_pRenderGraph != nullptr)
		{
			m_pRenderGraph->SetRenderStageSleeping(renderStageName, sleeping);
		}
		else
		{
			LOG_WARNING("[RenderSystem]: SetRenderStageSleeping failed - Rendergraph not initilised");
		}

	}

	glm::mat4 RenderSystem::CreateEntityTransform(Entity entity, const glm::bvec3& rotationalAxes)
	{
		const ECSCore* pECSCore	= ECSCore::GetInstance();
		const PositionComponent& positionComp	= pECSCore->GetConstComponent<PositionComponent>(entity);
		const RotationComponent& rotationComp	= pECSCore->GetConstComponent<RotationComponent>(entity);
		const ScaleComponent& scaleComp			= pECSCore->GetConstComponent<ScaleComponent>(entity);

		return CreateEntityTransform(positionComp, rotationComp, scaleComp, rotationalAxes);
	}

	glm::mat4 RenderSystem::CreateEntityTransform(const PositionComponent& positionComp, const RotationComponent& rotationComp, const ScaleComponent& scaleComp, const glm::bvec3& rotationalAxes)
	{
		glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), positionComp.Position);

		if (rotationalAxes.x && rotationalAxes.y && rotationalAxes.z)
		{
			transform = transform * glm::toMat4(rotationComp.Quaternion);
		}
		else if (rotationalAxes.x || rotationalAxes.y || rotationalAxes.z)
		{
			glm::quat rotation	= rotationComp.Quaternion;
			rotation.x			*= rotationalAxes.x;
			rotation.y			*= rotationalAxes.y;
			rotation.z			*= rotationalAxes.z;
			rotation			= glm::normalize(rotation);
			transform			= transform * glm::toMat4(rotation);
		}

		transform			= glm::scale(transform, scaleComp.Scale);
		return transform;
	}

	void RenderSystem::OnStaticMeshEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity, glm::bvec3(true));
		AddRenderableEntity(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false);
	}

	void RenderSystem::OnAnimatedEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity, glm::bvec3(true));
		AddRenderableEntity(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, true);
	}

	void RenderSystem::OnAnimationAttachedEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);
		auto& animationAttachedComponent = pECSCore->GetComponent<AnimationAttachedComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity, glm::bvec3(false, true, false));
		transform = transform * animationAttachedComponent.Transform;

		AddRenderableEntity(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false);
	}

	void RenderSystem::OnPlayerEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity, glm::bvec3(false, true, false));
		AddRenderableEntity(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, true);
	}

	void RenderSystem::OnDirectionalEntityAdded(Entity entity)
	{
		if (!m_DirectionalExist)
		{
			ECSCore* pECSCore = ECSCore::GetInstance();

			const auto& dirLight = pECSCore->GetConstComponent<DirectionalLightComponent>(entity);
			const auto& position = pECSCore->GetConstComponent<PositionComponent>(entity);
			const auto& rotation = pECSCore->GetConstComponent<RotationComponent>(entity);

			UpdateDirectionalLight(
				dirLight.ColorIntensity,
				position.Position,
				rotation.Quaternion,
				dirLight.FrustumWidth,
				dirLight.FrustumHeight,
				dirLight.FrustumZNear,
				dirLight.FrustumZFar
			);

			m_DirectionalExist = true;
			m_LightsBufferDirty = true;
		}
		else
		{
			LOG_WARNING("Multiple directional lights not supported!");
		}
	}

	void RenderSystem::OnPointLightEntityAdded(Entity entity)
	{
		const ECSCore* pECSCore = ECSCore::GetInstance();

		const auto& pointLight = pECSCore->GetConstComponent<PointLightComponent>(entity);
		const auto& position = pECSCore->GetConstComponent<PositionComponent>(entity);

		uint32 pointLightIndex = m_PointLights.GetSize();
		m_EntityToPointLight[entity] = pointLightIndex;
		m_PointLightToEntity[pointLightIndex] = entity;

		m_PointLights.PushBack(PointLight{.ColorIntensity = pointLight.ColorIntensity, .Position = position.Position});

		if (m_RemoveTexturesOnDeletion || m_FreeTextureIndices.IsEmpty())
		{
			m_PointLights.GetBack().TextureIndex = pointLightIndex;
		}
		else
		{
			// Check for free texture index instead of creating new index
			uint32 textureIndex = m_FreeTextureIndices.GetBack();
			m_FreeTextureIndices.PopBack();

			m_PointLights.GetBack().TextureIndex = textureIndex;
		}
	}

	void RenderSystem::OnDirectionalEntityRemoved(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		m_LightBufferData.DirL_ColorIntensity = glm::vec4(0.f);
		m_DirectionalExist = false;
		m_LightsResourceDirty = true;
	}

	void RenderSystem::OnPointLightEntityRemoved(Entity entity)
	{
		if (m_PointLights.IsEmpty())
			return;

		uint32 lastIndex = m_PointLights.GetSize() - 1U;
		uint32 lastEntity = m_PointLightToEntity[lastIndex];
		uint32 currentIndex = m_EntityToPointLight[entity];

		uint32 freeTexIndex = m_PointLights[currentIndex].TextureIndex;
		m_PointLights[currentIndex] = m_PointLights[lastIndex];

		m_EntityToPointLight[lastEntity] = currentIndex;
		m_PointLightToEntity[currentIndex] = lastEntity;

		m_PointLightToEntity.erase(lastIndex);
		m_EntityToPointLight.erase(entity);
		m_PointLights.PopBack();

		if (!m_RemoveTexturesOnDeletion)
		{
			// Free Texture for new point lights
			m_FreeTextureIndices.PushBack(freeTexIndex);
		}
		else
		{
			// Update all point lights shadowmaps to handle removal of texture
			for (uint32 i = 0; i < m_PointLights.GetSize(); i++)
			{
					LightUpdateData lightUpdateData = {};
					lightUpdateData.PointLightIndex = i;
					lightUpdateData.TextureIndex = m_PointLights[i].TextureIndex;
					m_PointLightTextureUpdateQueue.PushBack(lightUpdateData);
			}

			m_PointLightsDirty = true;
		}

		m_LightsBufferDirty = true;
	}

	void RenderSystem::OnEmitterEntityRemoved(Entity entity)
	{
		m_ParticleManager.OnEmitterEntityRemoved(entity);
	}
	
	void RenderSystem::AddRenderableEntity(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isAnimated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		uint32 extensionIndex = 0;
		uint32 materialIndex = UINT32_MAX;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID	= meshGUID;
		meshKey.IsAnimated	= isAnimated;
		meshKey.EntityID	= entity;
		meshKey.EntityMask	= EntityMaskManager::FetchEntityMask(entity);

		LOG_INFO("Entity Added with Mask: %x", meshKey.EntityMask);

		//Get meshAndInstancesIterator
		{
			meshAndInstancesIt = m_MeshAndInstancesMap.find(meshKey);
			if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
			{
				const Mesh* pMesh = ResourceManager::GetMesh(meshGUID);
				VALIDATE(pMesh != nullptr);

				MeshEntry meshEntry = {};

				// Vertices
				{
					BufferDesc vertexStagingBufferDesc = {};
					vertexStagingBufferDesc.DebugName	= "Vertex Staging Buffer";
					vertexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					vertexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					vertexStagingBufferDesc.SizeInBytes = pMesh->Vertices.GetSize() * sizeof(Vertex);

					Buffer* pVertexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexStagingBufferDesc);
					VALIDATE(pVertexStagingBuffer != nullptr);

					void* pMapped = pVertexStagingBuffer->Map();
					memcpy(pMapped, pMesh->Vertices.GetData(), vertexStagingBufferDesc.SizeInBytes);
					pVertexStagingBuffer->Unmap();

					BufferDesc vertexBufferDesc = {};
					vertexBufferDesc.DebugName		= "Vertex Buffer";
					vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					vertexBufferDesc.SizeInBytes	= vertexStagingBufferDesc.SizeInBytes;

					meshEntry.pVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);
					meshEntry.VertexCount	= pMesh->Vertices.GetSize();
					VALIDATE(meshEntry.pVertexBuffer != nullptr);

					if (isAnimated)
					{
						vertexBufferDesc.DebugName		= "Animated Vertices Buffer";
						meshEntry.pAnimatedVertexBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexBufferDesc);
						VALIDATE(meshEntry.pAnimatedVertexBuffer != nullptr);

						BufferDesc vertexWeightBufferDesc = {};
						vertexWeightBufferDesc.DebugName	= "Vertex Weight Staging Buffer";
						vertexWeightBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						vertexWeightBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						vertexWeightBufferDesc.SizeInBytes	= pMesh->VertexJointData.GetSize() * sizeof(VertexJointData);

						VALIDATE(pMesh->VertexJointData.GetSize() == pMesh->Vertices.GetSize());

						Buffer* pVertexWeightStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexWeightBufferDesc);
						VALIDATE(pVertexWeightStagingBuffer != nullptr);

						void* pMappedWeights = pVertexWeightStagingBuffer->Map();
						memcpy(pMappedWeights, pMesh->VertexJointData.GetData(), vertexWeightBufferDesc.SizeInBytes);
						pVertexWeightStagingBuffer->Unmap();

						vertexWeightBufferDesc.DebugName	= "Vertex Weight Buffer";
						vertexWeightBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						vertexWeightBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;

						meshEntry.pVertexWeightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexWeightBufferDesc);
						VALIDATE(meshEntry.pVertexWeightsBuffer != nullptr);

						m_PendingBufferUpdates.PushBack({ pVertexWeightStagingBuffer, 0, meshEntry.pVertexWeightsBuffer, 0, vertexWeightBufferDesc.SizeInBytes });
						DeleteDeviceResource(pVertexWeightStagingBuffer);
					}

					m_PendingBufferUpdates.PushBack({ pVertexStagingBuffer, 0, meshEntry.pVertexBuffer, 0, vertexBufferDesc.SizeInBytes });
					DeleteDeviceResource(pVertexStagingBuffer);
				}

				// Indices
				{
					BufferDesc indexStagingBufferDesc = {};
					indexStagingBufferDesc.DebugName	= "Index Staging Buffer";
					indexStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					indexStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					indexStagingBufferDesc.SizeInBytes	= pMesh->Indices.GetSize() * sizeof(MeshIndexType);

					Buffer* pIndexStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&indexStagingBufferDesc);
					VALIDATE(pIndexStagingBuffer != nullptr);

					void* pMapped = pIndexStagingBuffer->Map();
					memcpy(pMapped, pMesh->Indices.GetData(), indexStagingBufferDesc.SizeInBytes);
					pIndexStagingBuffer->Unmap();

					BufferDesc indexBufferDesc = {};
					indexBufferDesc.DebugName	= "Index Buffer";
					indexBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
					indexBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					indexBufferDesc.SizeInBytes	= indexStagingBufferDesc.SizeInBytes;

					meshEntry.pIndexBuffer	= RenderAPI::GetDevice()->CreateBuffer(&indexBufferDesc);
					meshEntry.IndexCount	= pMesh->Indices.GetSize();
					VALIDATE(meshEntry.pIndexBuffer != nullptr);

					m_PendingBufferUpdates.PushBack({ pIndexStagingBuffer, 0, meshEntry.pIndexBuffer, 0, indexBufferDesc.SizeInBytes });
					DeleteDeviceResource(pIndexStagingBuffer);
				}

				if (m_MeshShadersEnabled)
				{
					// Meshlet
					{
						BufferDesc meshletStagingBufferDesc = {};
						meshletStagingBufferDesc.DebugName		= "Meshlet Staging Buffer";
						meshletStagingBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						meshletStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						meshletStagingBufferDesc.SizeInBytes	= pMesh->Meshlets.GetSize() * sizeof(Meshlet);

						Buffer* pMeshletStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&meshletStagingBufferDesc);
						VALIDATE(pMeshletStagingBuffer != nullptr);

						void* pMapped = pMeshletStagingBuffer->Map();
						memcpy(pMapped, pMesh->Meshlets.GetData(), meshletStagingBufferDesc.SizeInBytes);
						pMeshletStagingBuffer->Unmap();

						BufferDesc meshletBufferDesc = {};
						meshletBufferDesc.DebugName		= "Meshlet Buffer";
						meshletBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						meshletBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
						meshletBufferDesc.SizeInBytes	= meshletStagingBufferDesc.SizeInBytes;

						meshEntry.pMeshlets = RenderAPI::GetDevice()->CreateBuffer(&meshletBufferDesc);
						meshEntry.MeshletCount = pMesh->Meshlets.GetSize();
						VALIDATE(meshEntry.pMeshlets != nullptr);

						m_PendingBufferUpdates.PushBack({ pMeshletStagingBuffer, 0, meshEntry.pMeshlets, 0, meshletBufferDesc.SizeInBytes });
						DeleteDeviceResource(pMeshletStagingBuffer);
					}

					// Unique Indices
					{
						BufferDesc uniqueIndicesStagingBufferDesc = {};
						uniqueIndicesStagingBufferDesc.DebugName	= "Unique Indices Staging Buffer";
						uniqueIndicesStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						uniqueIndicesStagingBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						uniqueIndicesStagingBufferDesc.SizeInBytes	= pMesh->UniqueIndices.GetSize() * sizeof(MeshIndexType);

						Buffer* pUniqueIndicesStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&uniqueIndicesStagingBufferDesc);
						VALIDATE(pUniqueIndicesStagingBuffer != nullptr);

						void* pMapped = pUniqueIndicesStagingBuffer->Map();
						memcpy(pMapped, pMesh->UniqueIndices.GetData(), uniqueIndicesStagingBufferDesc.SizeInBytes);
						pUniqueIndicesStagingBuffer->Unmap();

						BufferDesc uniqueIndicesBufferDesc = {};
						uniqueIndicesBufferDesc.DebugName	= "Unique Indices Buffer";
						uniqueIndicesBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						uniqueIndicesBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
						uniqueIndicesBufferDesc.SizeInBytes	= uniqueIndicesStagingBufferDesc.SizeInBytes;

						meshEntry.pUniqueIndices = RenderAPI::GetDevice()->CreateBuffer(&uniqueIndicesBufferDesc);
						meshEntry.UniqueIndexCount = pMesh->UniqueIndices.GetSize();
						VALIDATE(meshEntry.pUniqueIndices != nullptr);

						m_PendingBufferUpdates.PushBack({ pUniqueIndicesStagingBuffer, 0, meshEntry.pUniqueIndices, 0, uniqueIndicesBufferDesc.SizeInBytes });
						DeleteDeviceResource(pUniqueIndicesStagingBuffer);
					}

					// Primitive indicies
					{
						BufferDesc primitiveIndicesStagingBufferDesc = {};
						primitiveIndicesStagingBufferDesc.DebugName		= "Primitive Indices Staging Buffer";
						primitiveIndicesStagingBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
						primitiveIndicesStagingBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
						primitiveIndicesStagingBufferDesc.SizeInBytes	= pMesh->PrimitiveIndices.GetSize() * sizeof(PackedTriangle);

						Buffer* pPrimitiveIndicesStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&primitiveIndicesStagingBufferDesc);
						VALIDATE(pPrimitiveIndicesStagingBuffer != nullptr);

						void* pMapped = pPrimitiveIndicesStagingBuffer->Map();
						memcpy(pMapped, pMesh->PrimitiveIndices.GetData(), primitiveIndicesStagingBufferDesc.SizeInBytes);
						pPrimitiveIndicesStagingBuffer->Unmap();

						BufferDesc primitiveIndicesBufferDesc = {};
						primitiveIndicesBufferDesc.DebugName	= "Primitive Indices Buffer";
						primitiveIndicesBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						primitiveIndicesBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
						primitiveIndicesBufferDesc.SizeInBytes	= primitiveIndicesStagingBufferDesc.SizeInBytes;

						meshEntry.pPrimitiveIndices = RenderAPI::GetDevice()->CreateBuffer(&primitiveIndicesBufferDesc);
						meshEntry.PrimtiveIndexCount = pMesh->PrimitiveIndices.GetSize();
						VALIDATE(meshEntry.pPrimitiveIndices != nullptr);

						m_PendingBufferUpdates.PushBack({ pPrimitiveIndicesStagingBuffer, 0, meshEntry.pPrimitiveIndices, 0, primitiveIndicesBufferDesc.SizeInBytes });
						DeleteDeviceResource(pPrimitiveIndicesStagingBuffer);
					}
				}

				// Add Draw Arg Extensions.
				{
					meshEntry.DrawArgsMask = meshKey.EntityMask;
					if (meshEntry.DrawArgsMask & ~EntityMaskManager::FetchDefaultEntityMask()) // If the entity has extensions, it will differ from the default mask, and then add them to the entry.
					{
						DrawArgExtensionGroup& extensionGroup = EntityMaskManager::GetExtensionGroup(entity);
						meshEntry.ExtensionGroups.PushBack(&extensionGroup);
						extensionIndex = meshEntry.ExtensionGroups.GetSize();
						meshEntry.HasExtensions = true;
					}
					else
					{
						meshEntry.ExtensionGroups.PushBack(nullptr);
						extensionIndex = 0;
					}
				}

				if (m_RayTracingEnabled)
				{
					m_pASBuilder->BuildTriBLAS(
						meshEntry.BLASIndex,
						0U,
						isAnimated ? meshEntry.pAnimatedVertexBuffer : meshEntry.pVertexBuffer,
						meshEntry.pIndexBuffer,
						meshEntry.VertexCount,
						sizeof(Vertex),
						meshEntry.IndexCount,
						isAnimated);
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;
			}
		}

		//Get Material Slot
		{
			THashTable<uint32, uint32>::iterator materialIndexIt = m_MaterialMap.find(materialGUID);

			//Push new Material if the Material is yet to be registered
			if (materialIndexIt == m_MaterialMap.end())
			{
				const Material* pMaterial = ResourceManager::GetMaterial(materialGUID);
				VALIDATE(pMaterial != nullptr);

				if (!m_ReleasedMaterialIndices.IsEmpty())
				{
					materialIndex = m_ReleasedMaterialIndices.GetBack();
					m_ReleasedMaterialIndices.PopBack();

					m_AlbedoMaps[materialIndex]					= pMaterial->pAlbedoMap;
					m_NormalMaps[materialIndex]					= pMaterial->pNormalMap;
					m_CombinedMaterialMaps[materialIndex]		= pMaterial->pAOMetallicRoughnessMap;

					m_AlbedoMapViews[materialIndex]				= pMaterial->pAlbedoMapView;
					m_NormalMapViews[materialIndex]				= pMaterial->pNormalMapView;
					m_CombinedMaterialMapViews[materialIndex]	= pMaterial->pAOMetallicRoughnessMapView;

					m_MaterialProperties[materialIndex]			= pMaterial->Properties;

					m_MaterialInstanceCounts[materialIndex]		= 0;

					m_MaterialMap.insert({ materialGUID, materialIndex });
				}
				else
				{
					materialIndex = m_AlbedoMaps.GetSize();

					m_AlbedoMaps.PushBack(pMaterial->pAlbedoMap);
					m_NormalMaps.PushBack(pMaterial->pNormalMap);
					m_CombinedMaterialMaps.PushBack(pMaterial->pAOMetallicRoughnessMap);

					m_AlbedoMapViews.PushBack(pMaterial->pAlbedoMapView);
					m_NormalMapViews.PushBack(pMaterial->pNormalMapView);
					m_CombinedMaterialMapViews.PushBack(pMaterial->pAOMetallicRoughnessMapView);

					m_MaterialProperties.PushBack(pMaterial->Properties);

					m_MaterialInstanceCounts.PushBack(0);

					m_MaterialMap.insert({ materialGUID, materialIndex });
				}

				m_MaterialsResourceDirty = true;
				m_MaterialsPropertiesBufferDirty = true;
			}
			else
			{
				materialIndex = materialIndexIt->second;
			}

			m_MaterialInstanceCounts[materialIndex]++;
		}

		// Update resource for the entity mesh paint textures that is used for ray tracing
		bool hasPaintMask = false;
		if (m_RayTracingEnabled)
		{
			ECSCore* pECS = ECSCore::GetInstance();
			const ComponentArray<MeshPaintComponent>* pMeshPaintComponents = pECS->GetComponentArray<MeshPaintComponent>();
			if (pMeshPaintComponents->HasComponent(entity))
			{
				hasPaintMask = true;
				const auto& comp = pECS->GetComponent<MeshPaintComponent>(entity);

				Texture* pTexture			= comp.pTexture;
				TextureView* pTextureView	= comp.pTextureView;

				// If the texture has not been added before, update resource
				auto paintMaskTexturesIt = std::find(m_PaintMaskTextures.begin(), m_PaintMaskTextures.end(), pTexture);
				if (paintMaskTexturesIt == m_PaintMaskTextures.end())
				{
					if (m_PaintMaskTextures.IsEmpty())
					{
						m_PaintMaskTextures.PushBack(ResourceManager::GetTexture(GUID_TEXTURE_DEFAULT_MASK_MAP));
						m_PaintMaskTextureViews.PushBack(ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_MASK_MAP));
					}

					m_PaintMaskTextures.PushBack(pTexture);
					m_PaintMaskTextureViews.PushBack(pTextureView);
				}
			}
		}

		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.RasterInstances.GetSize();
		m_EntityIDsToInstanceKey[entity] = instanceKey;

		if (m_RayTracingEnabled)
		{
			RayTracedComponent rayTracedComponent = {};
			ECSCore::GetInstance()->GetComponentArray<RayTracedComponent>()->GetConstIf(entity, rayTracedComponent);

			uint32 shiftedMaterialIndex	= (materialIndex & 0xFF) << 8;
			uint32 paintIndex			= m_PaintMaskTextures.GetSize() - 1;
			uint32 shiftedPaintIndex	= hasPaintMask ? (std::max(0u, paintIndex)) & 0xFF : 0;

			uint32 customIndex =
				shiftedMaterialIndex |
				shiftedPaintIndex;
			FAccelerationStructureFlags asFlags	= RAY_TRACING_INSTANCE_FLAG_FORCE_OPAQUE | RAY_TRACING_INSTANCE_FLAG_FRONT_CCW;

			ASInstanceDesc asInstanceDesc =
			{
				.BlasIndex		= meshAndInstancesIt->second.BLASIndex,
				.Transform		= transform,
				.CustomIndex	= customIndex,
				.HitMask		= rayTracedComponent.HitMask,
				.Flags			= asFlags
			};

			uint32 asInstanceIndex = m_pASBuilder->AddInstance(asInstanceDesc);

			if (hasPaintMask)
			{
				m_PaintMaskASInstanceIndices[paintIndex].PushBack(asInstanceIndex);
			}

			LOG_MESSAGE("New AS: %u", shiftedPaintIndex);
			m_pASBuilder->UpdateInstances(
				[](AccelerationStructureInstance& asInstance)
				{
					uint32 asTextureIndex = asInstance.CustomIndex & 0xFF;
					LOG_MESSAGE("AS: %u", asTextureIndex);
				});

			meshAndInstancesIt->second.ASInstanceIndices.PushBack(asInstanceIndex);
		}

		Instance instance = {};
		instance.Transform		= transform;
		instance.PrevTransform	= transform;
		instance.ExtensionIndex = extensionIndex;
		instance.MaterialIndex	= materialIndex;
		instance.MeshletCount	= meshAndInstancesIt->second.MeshletCount;
		meshAndInstancesIt->second.RasterInstances.PushBack(instance);

		meshAndInstancesIt->second.EntityIDs.PushBack(entity);

		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);

		for (const DrawArgMaskDesc& maskDesc : m_RequiredDrawArgs)
		{
			if ((meshAndInstancesIt->second.DrawArgsMask & maskDesc.IncludeMask) == maskDesc.IncludeMask && (meshAndInstancesIt->second.DrawArgsMask & maskDesc.ExcludeMask) == 0)
			{
				m_DirtyDrawArgs.insert(maskDesc);
			}
		}
	}

	void RenderSystem::RemoveRenderableEntity(Entity entity)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);
		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which does not exist");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);
		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to remove entity which has no MeshAndInstancesMap entry");
			return;
		}

		const uint32 instanceIndex = instanceKeyIt->second.InstanceIndex;
		TArray<RenderSystem::Instance>& rasterInstances = meshAndInstancesIt->second.RasterInstances;
		const Instance& rasterInstance = rasterInstances[instanceIndex];

		//Update Material Instance Counts
		{
			uint32& materialInstanceCount = m_MaterialInstanceCounts[rasterInstance.MaterialIndex];
			materialInstanceCount--;

			if (materialInstanceCount == 0)
			{
				//Mark material as empty
				m_ReleasedMaterialIndices.PushBack(rasterInstance.MaterialIndex);
				auto materialToRemoveIt = std::find_if(m_MaterialMap.begin(), m_MaterialMap.end(), [rasterInstance](const std::pair<GUID_Lambda, uint32>& pair) {return rasterInstance.MaterialIndex == pair.second; });

				if (materialToRemoveIt != m_MaterialMap.end())
				{
					m_MaterialMap.erase(materialToRemoveIt);
				}
			}
		}

		if (m_RayTracingEnabled)
		{
			// Remove ASInstance
			uint32 asInstanceIndex = meshAndInstancesIt->second.ASInstanceIndices[instanceIndex];
			const uint32 textureIndex = m_pASBuilder->GetInstance(asInstanceIndex).CustomIndex & 0xFF;
			m_pASBuilder->RemoveInstance(asInstanceIndex);
			
			//Swap Removed with Back
			meshAndInstancesIt->second.ASInstanceIndices[instanceIndex] = meshAndInstancesIt->second.ASInstanceIndices.GetBack();
			meshAndInstancesIt->second.ASInstanceIndices.PopBack();

			// Remove and reorder the paint mask textures (if needed) and set new indicies for ASInstances
			ECSCore* pECS = ECSCore::GetInstance();
			const ComponentArray<MeshPaintComponent>* pMeshPaintComponents = pECS->GetComponentArray<MeshPaintComponent>();
			if (pMeshPaintComponents->HasComponent(entity))
			{
				uint32 changedIndex = m_PaintMaskTextures.GetSize() - 1;
				m_PaintMaskTextures[textureIndex]		= m_PaintMaskTextures.GetBack();
				m_PaintMaskTextures.PopBack();
				m_PaintMaskTextureViews[textureIndex]	= m_PaintMaskTextureViews.GetBack();
				m_PaintMaskTextureViews.PopBack();

				TArray<uint32>& asInstanceToBeRemoved = m_PaintMaskASInstanceIndices[textureIndex];
				if (auto asInstanceIt = std::find(asInstanceToBeRemoved.Begin(), asInstanceToBeRemoved.End(), asInstanceIndex); asInstanceIt != asInstanceToBeRemoved.End())
				{
					asInstanceToBeRemoved.Erase(asInstanceIt);
				}

				TArray<uint32>& asInstancesToBeUpdated = m_PaintMaskASInstanceIndices[changedIndex];
				for (uint32 asInstanceIndexToBeUpdated : asInstancesToBeUpdated)
				{
					m_pASBuilder->UpdateInstance(
						asInstanceIndexToBeUpdated,
						[textureIndex](AccelerationStructureInstance& asInstance)
						{
							asInstance.CustomIndex &= 0xFFFF00;
							asInstance.CustomIndex |= textureIndex;
						});

					asInstanceToBeRemoved.PushBack(asInstanceIndexToBeUpdated);
				}
				asInstancesToBeUpdated.Clear();

				m_RayTracingPaintMaskTexturesResourceDirty = true;
			}
		}

		// Remove extension
		{
			// Fetch the current instance and its extension index.
			const Instance& currentInstance = rasterInstances[instanceIndex];
			uint32 extensionIndex = currentInstance.ExtensionIndex;

			// extensionIndex == 0 means the mesh instance does not have an extension
			if (extensionIndex != 0)
			{
				// Set the last entity to use the extension group at the previous removed entity position.
				Entity swappedEntityID = meshAndInstancesIt->second.EntityIDs.GetBack();
				const InstanceKey& instanceKey = m_EntityIDsToInstanceKey[swappedEntityID];
				Instance& instance = rasterInstances[instanceKey.InstanceIndex];
				instance.ExtensionIndex = extensionIndex;

				// Remove the group in the list and replace it with the last group.
				TArray<DrawArgExtensionGroup*>& extensionGroups = meshAndInstancesIt->second.ExtensionGroups;
				extensionGroups[instanceIndex] = extensionGroups.GetBack();
				extensionGroups.PopBack();

				// Remove data from the storage.
				EntityMaskManager::RemoveAllExtensionsFromEntity(entity);
			}
		}

		rasterInstances[instanceIndex] = rasterInstances.GetBack();
		rasterInstances.PopBack();
		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);

		Entity swappedEntityID = meshAndInstancesIt->second.EntityIDs.GetBack();
		meshAndInstancesIt->second.EntityIDs[instanceIndex] = swappedEntityID;
		meshAndInstancesIt->second.EntityIDs.PopBack();

		auto swappedInstanceKeyIt = m_EntityIDsToInstanceKey.find(swappedEntityID);
		swappedInstanceKeyIt->second.InstanceIndex = instanceKeyIt->second.InstanceIndex;
		m_EntityIDsToInstanceKey.erase(instanceKeyIt);

		m_DirtyDrawArgs = m_RequiredDrawArgs;

		// Unload Mesh, Todo: Should we always do this?
		if (meshAndInstancesIt->second.EntityIDs.IsEmpty())
		{
			DeleteDeviceResource(meshAndInstancesIt->second.pVertexBuffer);
			DeleteDeviceResource(meshAndInstancesIt->second.pIndexBuffer);
			DeleteDeviceResource(meshAndInstancesIt->second.pUniqueIndices);
			DeleteDeviceResource(meshAndInstancesIt->second.pPrimitiveIndices);
			DeleteDeviceResource(meshAndInstancesIt->second.pMeshlets);
			DeleteDeviceResource(meshAndInstancesIt->second.pRasterInstanceBuffer);

			if (meshAndInstancesIt->second.pAnimatedVertexBuffer)
			{
				VALIDATE(meshAndInstancesIt->second.pAnimatedVertexBuffer);
				DeleteDeviceResource(meshAndInstancesIt->second.pAnimatedVertexBuffer);

				VALIDATE(meshAndInstancesIt->second.pAnimationDescriptorSet);
				DeleteDeviceResource(meshAndInstancesIt->second.pAnimationDescriptorSet);

				VALIDATE(meshAndInstancesIt->second.pBoneMatrixBuffer);
				DeleteDeviceResource(meshAndInstancesIt->second.pBoneMatrixBuffer);

				VALIDATE(meshAndInstancesIt->second.pVertexWeightsBuffer);
				DeleteDeviceResource(meshAndInstancesIt->second.pVertexWeightsBuffer);

				VALIDATE(meshAndInstancesIt->second.pStagingMatrixBuffer);
				DeleteDeviceResource(meshAndInstancesIt->second.pStagingMatrixBuffer);
			}

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				DeleteDeviceResource(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}

			auto dirtyRasterInstanceToRemove = std::find_if(m_DirtyRasterInstanceBuffers.begin(), m_DirtyRasterInstanceBuffers.end(), [meshAndInstancesIt](const MeshEntry* pMeshEntry)
				{
					return pMeshEntry == &meshAndInstancesIt->second;
				});

			if (dirtyRasterInstanceToRemove != m_DirtyRasterInstanceBuffers.end())
				m_DirtyRasterInstanceBuffers.erase(dirtyRasterInstanceToRemove);

			if (m_RayTracingEnabled)
			{
				m_pASBuilder->ReleaseBLAS(meshAndInstancesIt->second.BLASIndex);
			}

			m_MeshAndInstancesMap.erase(meshAndInstancesIt);
		}
	}

	void RenderSystem::UpdateParticleEmitter(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ParticleEmitterComponent& emitterComp)
	{
		m_ParticleManager.UpdateParticleEmitter(entity, positionComp, rotationComp, emitterComp);
	}

	void RenderSystem::UpdateDirectionalLight(const glm::vec4& colorIntensity, const glm::vec3& position, const glm::quat& direction, float frustumWidth, float frustumHeight, float zNear, float zFar)
	{
		m_LightBufferData.DirL_ColorIntensity	= colorIntensity;
		m_LightBufferData.DirL_Direction = -GetForward(direction);

		glm::mat4 lightView = glm::lookAt(position, position - m_LightBufferData.DirL_Direction, g_DefaultUp);
		glm::mat4 lightProj = glm::ortho(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, zNear, zFar);
		m_LightBufferData.DirL_ProjViews = lightProj * lightView;

		m_pRenderGraph->TriggerRenderStage("DIRL_SHADOWMAP");
		m_LightsBufferDirty = true;
	}

	void RenderSystem::UpdatePointLight(Entity entity, const glm::vec3& position, const glm::vec4& colorIntensity, float nearPlane, float farPlane)
	{
		if (m_EntityToPointLight.find(entity) == m_EntityToPointLight.end())
		{
			LOG_ERROR("Entity non-existing in PointLight map!");
			return;
		}
		uint32 index = m_EntityToPointLight[entity];
		PointLight& pointLight = m_PointLights[index];
		pointLight.ColorIntensity = colorIntensity;

		m_LightsBufferDirty = true;
		m_PointLightsDirty = true;

		if (pointLight.Position != position
			|| pointLight.FarPlane != farPlane
			|| pointLight.NearPlane != nearPlane)
		{
			pointLight.Position = position;

			const glm::vec3 directions[6] =
			{
				{1.0f, 0.0f, 0.0f},
				{-1.0f, 0.0f, 0.0f},
				{0.0f, 1.0f, 0.0f},
				{0.0f, -1.0f, 0.0f},
				{0.0f, 0.0f, 1.0f},
				{0.0f, 0.0f, -1.0f},
			};

			const glm::vec3 defaultUp[6] =
			{
				-g_DefaultUp,
				-g_DefaultUp,
				-g_DefaultForward,
				g_DefaultForward,
				-g_DefaultUp,
				-g_DefaultUp,
			};

			constexpr uint32 PROJECTIONS = 6;
			constexpr float FOV = 90.f;
			constexpr float ASPECT_RATIO = 1.0f;
			pointLight.FarPlane = farPlane;

			glm::mat4 perspective = glm::perspective(glm::radians(FOV), ASPECT_RATIO, nearPlane, farPlane);
			// Create projection matrices for each face
			for (uint32 p = 0; p < PROJECTIONS; p++)
			{
				pointLight.ProjViews[p] = perspective;
				pointLight.ProjViews[p] *= glm::lookAt(position, position + directions[p], defaultUp[p]);
			}

			LightUpdateData lightTextureUpdate = {};
			lightTextureUpdate.PointLightIndex = index;
			lightTextureUpdate.TextureIndex = m_PointLights[index].TextureIndex;
			m_PointLightTextureUpdateQueue.PushBack(lightTextureUpdate);

			m_PointLightsDirty = true;
		}
	}

	void RenderSystem::UpdateAnimation(Entity entity, MeshComponent& meshComp, AnimationComponent& animationComp)
	{
		if (animationComp.IsPaused)
			return;

		MeshKey key(meshComp.MeshGUID, entity, true, EntityMaskManager::FetchEntityMask(entity));

		auto meshEntryIt = m_MeshAndInstancesMap.find(key);
		if (meshEntryIt != m_MeshAndInstancesMap.end())
		{
			UpdateAnimationBuffers(animationComp, meshEntryIt->second);

			MeshEntry* pMeshEntry = &meshEntryIt->second;
			m_AnimationsToUpdate.insert(pMeshEntry);

			if (m_RayTracingEnabled)
			{
				m_pASBuilder->BuildTriBLAS(
					pMeshEntry->BLASIndex,
					0U,
					pMeshEntry->pAnimatedVertexBuffer,
					pMeshEntry->pIndexBuffer,
					pMeshEntry->VertexCount,
					sizeof(Vertex),
					pMeshEntry->IndexCount,
					true);
			}
		}
	}

	void RenderSystem::UpdateTransform(Entity entity, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ScaleComponent& scaleComp, const glm::bvec3& rotationalAxes)
	{
		if (!positionComp.Dirty && !rotationComp.Dirty && !scaleComp.Dirty)
			return;

		glm::mat4 transform = CreateEntityTransform(positionComp, rotationComp, scaleComp, rotationalAxes);

		UpdateTransformData(entity, transform);
	}

	void RenderSystem::UpdateTransform(Entity entity, const glm::mat4& additionalTransform, const PositionComponent& positionComp, const RotationComponent& rotationComp, const ScaleComponent& scaleComp, const glm::bvec3& rotationalAxes)
	{
		if (!positionComp.Dirty && !rotationComp.Dirty && !scaleComp.Dirty)
			return;

		glm::mat4 transform = CreateEntityTransform(positionComp, rotationComp, scaleComp, rotationalAxes);
		transform = transform * additionalTransform;

		UpdateTransformData(entity, transform);
	}

	void RenderSystem::UpdateTransformData(Entity entity, const glm::mat4& transform)
	{
		THashTable<GUID_Lambda, InstanceKey>::iterator instanceKeyIt = m_EntityIDsToInstanceKey.find(entity);
		if (instanceKeyIt == m_EntityIDsToInstanceKey.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which is not registered");
			return;
		}

		MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.find(instanceKeyIt->second.MeshKey);
		if (meshAndInstancesIt == m_MeshAndInstancesMap.end())
		{
			LOG_ERROR("[RenderSystem]: Tried to update transform of an entity which has no MeshAndInstancesMap entry");
			return;
		}

		if (m_RayTracingEnabled)
		{
			uint32 asInstanceIndex = meshAndInstancesIt->second.ASInstanceIndices[instanceKeyIt->second.InstanceIndex];
			m_pASBuilder->UpdateInstanceTransform(asInstanceIndex, transform);
		}

		Instance* pRasterInstanceToUpdate = &meshAndInstancesIt->second.RasterInstances[instanceKeyIt->second.InstanceIndex];
		pRasterInstanceToUpdate->PrevTransform	= pRasterInstanceToUpdate->Transform;
		pRasterInstanceToUpdate->Transform		= transform;
		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);
	}

	void RenderSystem::UpdateCamera(const glm::vec3& position, const glm::quat& rotation, const CameraComponent& camComp, const ViewProjectionMatricesComponent& viewProjComp)
	{
		m_PerFrameData.CamData.PrevView			= m_PerFrameData.CamData.View;
		m_PerFrameData.CamData.PrevProjection	= m_PerFrameData.CamData.Projection;
		m_PerFrameData.CamData.View				= viewProjComp.View;
		m_PerFrameData.CamData.Projection		= viewProjComp.Projection;
		m_PerFrameData.CamData.ViewInv			= camComp.ViewInv;
		m_PerFrameData.CamData.ProjectionInv	= camComp.ProjectionInv;
		m_PerFrameData.CamData.Position			= glm::vec4(position, 0.f);
		m_PerFrameData.CamData.Up				= glm::vec4(GetUp(rotation), 0.f);
		m_PerFrameData.CamData.Jitter			= camComp.Jitter;
	}

	void RenderSystem::DeleteDeviceResource(DeviceChild* pDeviceResource)
	{
		m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDeviceResource);
	}

	void RenderSystem::CleanBuffers()
	{
		// Todo: Better solution for this, save some Staging Buffers maybe so they don't get recreated all the time?
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_ModFrameIndex];
		for (uint32 i = 0; i < resourcesToRemove.GetSize(); i++)
		{
			DeviceChild* pResource = resourcesToRemove[i];
			SAFERELEASE(pResource);
		}

		resourcesToRemove.Clear();
	}

	void RenderSystem::CreateDrawArgs(TArray<DrawArg>& drawArgs, const DrawArgMaskDesc& requestedMaskDesc) const
	{
		for (auto& meshEntryPair : m_MeshAndInstancesMap)
		{
			uint32 mask = meshEntryPair.second.DrawArgsMask;
			if ((mask & requestedMaskDesc.IncludeMask) == requestedMaskDesc.IncludeMask && (mask & requestedMaskDesc.ExcludeMask) == 0)
			{
				DrawArg drawArg = { };

				// Get all entites that are using this mesh
				drawArg.EntityIDs = meshEntryPair.second.EntityIDs;

				// Assume animated
				if (meshEntryPair.second.pAnimatedVertexBuffer)
				{
					drawArg.pVertexBuffer = meshEntryPair.second.pAnimatedVertexBuffer;
				}
				else
				{
					drawArg.pVertexBuffer = meshEntryPair.second.pVertexBuffer;
				}

				drawArg.pIndexBuffer = meshEntryPair.second.pIndexBuffer;
				drawArg.IndexCount = meshEntryPair.second.IndexCount;

				drawArg.pInstanceBuffer = meshEntryPair.second.pRasterInstanceBuffer;
				drawArg.InstanceCount = meshEntryPair.second.RasterInstances.GetSize();

				drawArg.pMeshletBuffer = meshEntryPair.second.pMeshlets;
				drawArg.MeshletCount = meshEntryPair.second.MeshletCount;
				drawArg.pUniqueIndicesBuffer = meshEntryPair.second.pUniqueIndices;
				drawArg.pPrimitiveIndices = meshEntryPair.second.pPrimitiveIndices;

				if (!meshEntryPair.second.ExtensionGroups.IsEmpty())
				{
					drawArg.ppExtensionGroups = meshEntryPair.second.ExtensionGroups.GetData();
					drawArg.HasExtensions = meshEntryPair.second.HasExtensions;
				}
				else
				{
					drawArg.HasExtensions = false;
				}

				drawArgs.PushBack(drawArg);
			}
		}
	}

	void RenderSystem::UpdateBuffers()
	{
		CommandList* pGraphicsCommandList	= m_pRenderGraph->AcquireGraphicsCopyCommandList();
		CommandList* pComputeCommandList	= m_pRenderGraph->AcquireComputeCopyCommandList();

		//Update Pending Buffer Updates
		{
			ExecutePendingBufferUpdates(pGraphicsCommandList);
		}

		//Update Per Frame Data
		{
			m_PerFrameData.FrameIndex = 0;
			m_PerFrameData.RandomSeed = uint32(Random::Int32(INT32_MIN, INT32_MAX));

			UpdatePerFrameBuffer(pGraphicsCommandList);
		}

		//Update Raster Instance Data
		{
			UpdateRasterInstanceBuffers(pGraphicsCommandList);
		}

		// Update Light Data
		{
			UpdateLightsBuffer(pGraphicsCommandList);
			UpdatePointLightTextureResource(pGraphicsCommandList);
		}

		// Update Paint Mask Color Data
		{
			UpdatePaintMaskColorBuffer(pGraphicsCommandList);
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList);
		}

		// Update particles
		{
			m_ParticleManager.UpdateBuffers(pGraphicsCommandList);
		}

		// Perform mesh skinning
		{
			PerformMeshSkinning(pComputeCommandList);
		}
	}

	void RenderSystem::UpdateAnimationBuffers(AnimationComponent& animationComp, MeshEntry& meshEntry)
	{
		// If needed create new buffers
		const uint64 sizeInBytes = animationComp.Pose.GlobalTransforms.GetSize() * sizeof(glm::mat4);
		if (animationComp.Pose.GlobalTransforms.GetSize() > meshEntry.BoneMatrixCount)
		{
			if (meshEntry.pBoneMatrixBuffer)
			{
				DeleteDeviceResource(meshEntry.pStagingMatrixBuffer);
				DeleteDeviceResource(meshEntry.pBoneMatrixBuffer);
				DeleteDeviceResource(meshEntry.pAnimationDescriptorSet);
			}

			BufferDesc matrixBufferDesc;
			matrixBufferDesc.DebugName		= "Matrix Staging Buffer";
			matrixBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			matrixBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			matrixBufferDesc.SizeInBytes	= sizeInBytes;

			meshEntry.pStagingMatrixBuffer = RenderAPI::GetDevice()->CreateBuffer(&matrixBufferDesc);

			matrixBufferDesc.DebugName	= "Matrix Buffer";
			matrixBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			matrixBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;

			meshEntry.pBoneMatrixBuffer			= RenderAPI::GetDevice()->CreateBuffer(&matrixBufferDesc);
			meshEntry.BoneMatrixCount			= animationComp.Pose.GlobalTransforms.GetSize();
			meshEntry.pAnimationDescriptorSet	= RenderAPI::GetDevice()->CreateDescriptorSet("Animation Descriptor Set", m_SkinningPipelineLayout.Get(), 0, m_AnimationDescriptorHeap.Get());

			const uint64 offset = 0;
			uint64 size = meshEntry.VertexCount * sizeof(Vertex);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pVertexBuffer,			&offset, &size,			0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pAnimatedVertexBuffer,	&offset, &size,			1, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pBoneMatrixBuffer,		&offset, &sizeInBytes,	2, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			size = meshEntry.VertexCount * sizeof(VertexJointData);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pVertexWeightsBuffer, &offset, &size, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
		}

		// Copy data
		void* pMapped = meshEntry.pStagingMatrixBuffer->Map();
		memcpy(pMapped, animationComp.Pose.GlobalTransforms.GetData(), sizeInBytes);
		meshEntry.pStagingMatrixBuffer->Unmap();

		m_PendingBufferUpdates.PushBack({ meshEntry.pStagingMatrixBuffer, 0, meshEntry.pBoneMatrixBuffer, 0, sizeInBytes });
	}

	void RenderSystem::PerformMeshSkinning(CommandList* pCommandList)
	{
		// TODO: Investigate the best groupsize for us (THIS MUST MATCH THE SHADER-DEFINE)
		constexpr uint32 THREADS_PER_WORKGROUP = 32;

		PipelineState* pPipeline = PipelineStateManager::GetPipelineState(m_SkinningPipelineID);
		VALIDATE(pPipeline != nullptr);

		for (MeshEntry* pMeshEntry : m_AnimationsToUpdate)
		{
			pCommandList->BindDescriptorSetCompute(pMeshEntry->pAnimationDescriptorSet, m_SkinningPipelineLayout.Get(), 0);
			pCommandList->BindComputePipeline(pPipeline);

			const uint32 vertexCount = pMeshEntry->VertexCount;
			pCommandList->SetConstantRange(m_SkinningPipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_COMPUTE_SHADER, &vertexCount, sizeof(uint32), 0);

			const uint32 workGroupCount = std::max<uint32>((uint32)AlignUp(vertexCount, THREADS_PER_WORKGROUP) / THREADS_PER_WORKGROUP, 1u);
			pCommandList->Dispatch(workGroupCount, 1, 1);
		}

		m_AnimationsToUpdate.clear();
	}

	void RenderSystem::ExecutePendingBufferUpdates(CommandList* pCommandList)
	{
		if (!m_PendingBufferUpdates.IsEmpty())
		{
			for (uint32 i = 0; i < m_PendingBufferUpdates.GetSize(); i++)
			{
				const PendingBufferUpdate& pendingUpdate = m_PendingBufferUpdates[i];
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, pendingUpdate.pDstBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			m_PendingBufferUpdates.Clear();
		}
	}

	void RenderSystem::UpdateRasterInstanceBuffers(CommandList* pCommandList)
	{
		for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyRasterInstanceBuffers)
		{
			//Raster Instances
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->RasterInstances.GetSize() * sizeof(Instance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr)
						DeleteDeviceResource(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName	= "Raster Instance Staging Buffer";
					bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes	= requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppRasterInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->RasterInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pRasterInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pRasterInstanceBuffer != nullptr)
						DeleteDeviceResource(pDirtyInstanceBufferEntry->pRasterInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= "Raster Instance Buffer";
					bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
					bufferDesc.SizeInBytes		= requiredBufferSize;

					pDirtyInstanceBufferEntry->pRasterInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pRasterInstanceBuffer, 0, requiredBufferSize);
			}
		}

		m_DirtyRasterInstanceBuffers.clear();
	}

	void RenderSystem::UpdatePerFrameBuffer(CommandList* pCommandList)
	{
		Buffer* pPerFrameStagingBuffer = m_ppPerFrameStagingBuffers[m_ModFrameIndex];

		void* pMapped = pPerFrameStagingBuffer->Map();
		memcpy(pMapped, &m_PerFrameData, sizeof(PerFrameBuffer));
		pPerFrameStagingBuffer->Unmap();

		pCommandList->CopyBuffer(pPerFrameStagingBuffer, 0, m_pPerFrameBuffer, 0, sizeof(PerFrameBuffer));
	}

	void RenderSystem::UpdateMaterialPropertiesBuffer(CommandList* pCommandList)
	{
		if (m_MaterialsPropertiesBufferDirty)
		{
			uint32 requiredBufferSize = m_MaterialProperties.GetSize() * sizeof(MaterialProperties);

			Buffer* pStagingBuffer = m_ppMaterialParametersStagingBuffers[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) DeleteDeviceResource(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Staging Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppMaterialParametersStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_MaterialProperties.GetData(), requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pMaterialParametersBuffer != nullptr) DeleteDeviceResource(m_pMaterialParametersBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				m_pMaterialParametersBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);

			m_MaterialsPropertiesBufferDirty = false;
		}
	}

	void RenderSystem::UpdatePointLightTextureResource(CommandList* pCommandList)
	{
		if (m_PointLightsDirty)
		{
			m_PointLightsDirty = false;
			m_LightsResourceDirty = true;

			uint32 pointLightCount = m_PointLights.GetSize();
			if (pointLightCount == m_CubeTextures.GetSize())
				return;

			bool needUpdate = m_RemoveTexturesOnDeletion;

			constexpr uint32 CUBE_FACE_COUNT = 6;
			if (pointLightCount > m_CubeTextures.GetSize())
			{
				GraphicsDevice* pGraphicsDevice = RenderAPI::GetDevice();
				uint32 diff = pointLightCount - m_CubeTextures.GetSize();

				// TODO: Create inteface for changing resolution
				const uint32 width = 512;
				const uint32 height = 512;

				uint32 prevSubImageCount = m_CubeSubImageTextureViews.GetSize();
				m_CubeSubImageTextureViews.Resize(prevSubImageCount + CUBE_FACE_COUNT * diff);

				for (uint32 c = 0; c < diff; c++)
				{
					// Create cube texture
					TextureDesc cubeTexDesc = {};
					cubeTexDesc.DebugName = "PointLight Texture Cube " + std::to_string(c);
					cubeTexDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
					cubeTexDesc.Format = EFormat::FORMAT_D24_UNORM_S8_UINT;
					cubeTexDesc.Type = ETextureType::TEXTURE_TYPE_2D;
					cubeTexDesc.Flags = FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL | FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_CUBE_COMPATIBLE;
					cubeTexDesc.Width = width;
					cubeTexDesc.Height = height;
					cubeTexDesc.Depth = 1U;
					cubeTexDesc.ArrayCount = 6;
					cubeTexDesc.Miplevels = 1;
					cubeTexDesc.SampleCount = 1;

					Texture* pCubeTexture = pGraphicsDevice->CreateTexture(&cubeTexDesc);
					m_CubeTextures.PushBack(pCubeTexture);

					// Create cube texture view
					TextureViewDesc cubeTexViewDesc = {};
					cubeTexViewDesc.DebugName = "PointLight Texture Cube View " + std::to_string(c);
					cubeTexViewDesc.pTexture = pCubeTexture;
					cubeTexViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL | FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
					cubeTexViewDesc.Format = cubeTexDesc.Format;
					cubeTexViewDesc.Type = ETextureViewType::TEXTURE_VIEW_TYPE_CUBE;
					cubeTexViewDesc.MiplevelCount = 1;
					cubeTexViewDesc.ArrayCount = 6;
					cubeTexViewDesc.Miplevel = 0U;
					cubeTexViewDesc.ArrayIndex = 0U;

					m_CubeTextureViews.PushBack(pGraphicsDevice->CreateTextureView(&cubeTexViewDesc)); // Used for reading from CubeTexture

					// Create per face texture views
					TextureViewDesc subImageTextureViewDesc = {};
					subImageTextureViewDesc.pTexture = pCubeTexture;
					subImageTextureViewDesc.Flags = FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL;
					subImageTextureViewDesc.Format = cubeTexDesc.Format;
					subImageTextureViewDesc.Type = ETextureViewType::TEXTURE_VIEW_TYPE_2D;
					subImageTextureViewDesc.Miplevel = cubeTexViewDesc.Miplevel;
					subImageTextureViewDesc.MiplevelCount = cubeTexViewDesc.MiplevelCount;
					subImageTextureViewDesc.ArrayCount = 1;

					TextureView** ppSubImageView = &m_CubeSubImageTextureViews[prevSubImageCount + (CUBE_FACE_COUNT) * c];
					for (uint32 si = 0; si < CUBE_FACE_COUNT; si++)
					{
						subImageTextureViewDesc.DebugName = "PointLight Sub Image Texture View " + std::to_string(si);
						subImageTextureViewDesc.ArrayIndex = si;

						(*ppSubImageView) = pGraphicsDevice->CreateTextureView(&subImageTextureViewDesc); // Used for writing to CubeTexture
						ppSubImageView++;
					}

					PipelineTextureBarrierDesc transitionToReadOnlyBarrier = { };
					transitionToReadOnlyBarrier.pTexture				= pCubeTexture;
					transitionToReadOnlyBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
					transitionToReadOnlyBarrier.StateAfter				= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
					transitionToReadOnlyBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
					transitionToReadOnlyBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
					transitionToReadOnlyBarrier.SrcMemoryAccessFlags	= 0;
					transitionToReadOnlyBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
					transitionToReadOnlyBarrier.TextureFlags			= cubeTexDesc.Flags;
					transitionToReadOnlyBarrier.Miplevel				= 0;
					transitionToReadOnlyBarrier.MiplevelCount			= cubeTexDesc.Miplevels;
					transitionToReadOnlyBarrier.ArrayIndex				= 0;
					transitionToReadOnlyBarrier.ArrayCount				= cubeTexDesc.ArrayCount;

					pCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToReadOnlyBarrier, 1);

					needUpdate = true;
				}
			}
			else if (pointLightCount < m_CubeTextures.GetSize())
			{
				if (m_RemoveTexturesOnDeletion)
				{
					uint32 diff =  m_CubeTextures.GetSize() - pointLightCount;

					// Remove Cube Texture Context for removed pointlights
					for (uint32 r = 0; r < diff; r++)
					{
						DeleteDeviceResource(m_CubeTextures.GetBack());
						m_CubeTextures.PopBack();

						DeleteDeviceResource(m_CubeTextureViews.GetBack());
						m_CubeTextureViews.PopBack();

						for (uint32 f = 0; f < CUBE_FACE_COUNT && !m_CubeSubImageTextureViews.IsEmpty(); f++)
						{
							DeleteDeviceResource(m_CubeSubImageTextureViews.GetBack());
							m_CubeSubImageTextureViews.PopBack();
						}
					}
				}
			}

			/*
				If textures are not removed on pointlight deletion(m_RemoveTexturesOnDeletion == false), updates are only needed when new point light textures are added
			*/
			if (needUpdate)
			{
				uint32 texturesExisting = m_CubeTextures.GetSize();
				Sampler* pNearestSampler = Sampler::GetNearestSampler();
				ResourceUpdateDesc resourceUpdateDesc = {};
				resourceUpdateDesc.ResourceName = SCENE_POINT_SHADOWMAPS;
				resourceUpdateDesc.ExternalTextureUpdate.ppTextures							= m_CubeTextures.GetData();
				resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews						= m_CubeTextureViews.GetData();
				resourceUpdateDesc.ExternalTextureUpdate.TextureCount						= texturesExisting;
				resourceUpdateDesc.ExternalTextureUpdate.ppPerSubImageTextureViews			= m_CubeSubImageTextureViews.GetData();
				resourceUpdateDesc.ExternalTextureUpdate.PerImageSubImageTextureViewCount	= CUBE_FACE_COUNT;
				resourceUpdateDesc.ExternalTextureUpdate.ppSamplers							= &pNearestSampler;
				resourceUpdateDesc.ExternalTextureUpdate.SamplerCount						= 1;
				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
			}
		}
	}

	void RenderSystem::UpdateLightsBuffer(CommandList* pCommandList)
	{
		// Light Buffer Initilization
		if (m_LightsBufferDirty)
		{
			m_LightsBufferDirty = false;

			size_t pointLightCount			= m_PointLights.GetSize();
			size_t dirLightBufferSize		= sizeof(LightBuffer);
			size_t pointLightsBufferSize	= sizeof(PointLight) * pointLightCount;
			size_t lightBufferSize			= dirLightBufferSize + pointLightsBufferSize;

			// Set point light count
			m_LightBufferData.PointLightCount = float32(pointLightCount);

			Buffer* pCurrentStagingBuffer = m_ppLightsStagingBuffer[m_ModFrameIndex];

			if (pCurrentStagingBuffer == nullptr || pCurrentStagingBuffer->GetDesc().SizeInBytes < lightBufferSize)
			{
				if (pCurrentStagingBuffer != nullptr) DeleteDeviceResource(pCurrentStagingBuffer);

				BufferDesc lightCopyBufferDesc = {};
				lightCopyBufferDesc.DebugName		= "Lights Copy Buffer";
				lightCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				lightCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				lightCopyBufferDesc.SizeInBytes		= lightBufferSize;

				pCurrentStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&lightCopyBufferDesc);
				m_ppLightsStagingBuffer[m_ModFrameIndex] = pCurrentStagingBuffer;
			}

			void* pMapped = pCurrentStagingBuffer->Map();
			memcpy(pMapped, &m_LightBufferData, dirLightBufferSize);
			if (pointLightsBufferSize > 0) memcpy((uint8*)pMapped + dirLightBufferSize, m_PointLights.GetData(), pointLightsBufferSize);
			pCurrentStagingBuffer->Unmap();

			if (m_pLightsBuffer == nullptr || m_pLightsBuffer->GetDesc().SizeInBytes < lightBufferSize)
			{
				if (m_pLightsBuffer != nullptr) DeleteDeviceResource(m_pLightsBuffer);

				BufferDesc lightBufferDesc = {};
				lightBufferDesc.DebugName		= "Lights Buffer";
				lightBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
				lightBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
				lightBufferDesc.SizeInBytes		= lightBufferSize;

				m_pLightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&lightBufferDesc);
				m_LightsResourceDirty = true;
			}

			pCommandList->CopyBuffer(pCurrentStagingBuffer, 0, m_pLightsBuffer, 0, lightBufferSize);
		}
	}

	void RenderSystem::UpdatePaintMaskColorBuffer(CommandList* pCommandList)
	{
		if (m_PaintMaskColorsResourceDirty)
		{
			uint32 bufferSize = m_PaintMaskColors.GetSize() * sizeof(glm::vec4);

			// Create or update staging buffer if needed
			Buffer* pStagingBuffer = m_ppPaintMaskColorStagingBuffers[m_ModFrameIndex];
			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < bufferSize)
			{
				if (pStagingBuffer != nullptr) DeleteDeviceResource(pStagingBuffer);

				BufferDesc copyBufferDesc = {};
				copyBufferDesc.DebugName		= "Paint Mask Color Copy Buffer";
				copyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				copyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				copyBufferDesc.SizeInBytes		= bufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&copyBufferDesc);
				m_ppPaintMaskColorStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			// Transfer data to staging buffer
			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_PaintMaskColors.GetData(), bufferSize);
			pStagingBuffer->Unmap();

			// Create or update actual GPU buffer if needed
			if (m_pPaintMaskColorBuffer == nullptr || m_pPaintMaskColorBuffer->GetDesc().SizeInBytes < bufferSize)
			{
				if (m_pPaintMaskColorBuffer != nullptr) DeleteDeviceResource(m_pPaintMaskColorBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName		= "Paint Mask Color Buffer";
				bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
				bufferDesc.SizeInBytes		= bufferSize;

				m_pPaintMaskColorBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			// Finally copy over the data to the buffer
			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pPaintMaskColorBuffer, 0, bufferSize);
		}
	}

	void RenderSystem::UpdateRenderGraph()
	{
		//Should we check for Draw Args to be removed here?

		if (!m_DirtyDrawArgs.empty())
		{
			for (const DrawArgMaskDesc& maskDesc : m_DirtyDrawArgs)
			{
				TArray<DrawArg> drawArgs;
				CreateDrawArgs(drawArgs, maskDesc);

				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc						= {};
				resourceUpdateDesc.ResourceName								= SCENE_DRAW_ARGS;
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsMaskDesc	= maskDesc;
				resourceUpdateDesc.ExternalDrawArgsUpdate.pDrawArgs			= drawArgs.GetData();
				resourceUpdateDesc.ExternalDrawArgsUpdate.Count				= drawArgs.GetSize();

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
			}

			m_DirtyDrawArgs.clear();
		}

		if (m_PerFrameResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PER_FRAME_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pPerFrameBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_PerFrameResourceDirty = false;
		}

		// Trigger LightRenderer
		if (!m_PointLightTextureUpdateQueue.IsEmpty())
		{
			m_pLightRenderer->PrepareTextureUpdates(m_PointLightTextureUpdateQueue);
			m_PointLightTextureUpdateQueue.Clear();
			m_pRenderGraph->TriggerRenderStage("RENDER_STAGE_LIGHT");
		}

		if (m_LightsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pLightsBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_LightsResourceDirty = false;
		}

		if (m_PaintMaskColorsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= PAINT_MASK_COLORS;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pPaintMaskColorBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			m_PaintMaskColorsResourceDirty = false;
		}

		if (m_RayTracingPaintMaskTexturesResourceDirty)
		{
			ResourceUpdateDesc unwrappedTextureUpdate = {};
			unwrappedTextureUpdate.ResourceName												= "PAINT_MASK_TEXTURES";
			unwrappedTextureUpdate.ExternalTextureUpdate.ppTextures							= m_PaintMaskTextures.GetData();
			unwrappedTextureUpdate.ExternalTextureUpdate.ppTextureViews						= m_PaintMaskTextureViews.GetData();
			unwrappedTextureUpdate.ExternalTextureUpdate.ppPerSubImageTextureViews			= nullptr;
			unwrappedTextureUpdate.ExternalTextureUpdate.PerImageSubImageTextureViewCount	= 0;
			unwrappedTextureUpdate.ExternalTextureUpdate.ppSamplers							= Sampler::GetNearestSamplerToBind();
			unwrappedTextureUpdate.ExternalTextureUpdate.TextureCount						= m_PaintMaskTextures.GetSize();
			unwrappedTextureUpdate.ExternalTextureUpdate.SamplerCount						= 1;

			RenderSystem::GetInstance().GetRenderGraph()->UpdateResource(&unwrappedTextureUpdate);

			m_RayTracingPaintMaskTexturesResourceDirty = false;
		}

		// Update Particle Resources
		{
			m_ParticleManager.UpdateResources(m_pRenderGraph);
		}


		if (m_MaterialsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pMaterialParametersBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			Sampler* pLinearSamplers = Sampler::GetLinearSampler();

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName							= SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_AlbedoMaps.GetData();
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_AlbedoMapViews.GetData();
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pLinearSamplers;
			albedoMapsUpdateDesc.ExternalTextureUpdate.TextureCount		= m_AlbedoMaps.GetSize();
			albedoMapsUpdateDesc.ExternalTextureUpdate.SamplerCount		= 1;

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName							= SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_NormalMaps.GetData();
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_NormalMapViews.GetData();
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= &pLinearSamplers;
			normalMapsUpdateDesc.ExternalTextureUpdate.TextureCount		= m_NormalMapViews.GetSize();
			normalMapsUpdateDesc.ExternalTextureUpdate.SamplerCount		= 1;

			ResourceUpdateDesc combinedMaterialMapsUpdateDesc = {};
			combinedMaterialMapsUpdateDesc.ResourceName								= SCENE_COMBINED_MATERIAL_MAPS;
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_CombinedMaterialMaps.GetData();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_CombinedMaterialMapViews.GetData();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= &pLinearSamplers;
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.TextureCount		= m_CombinedMaterialMaps.GetSize();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.SamplerCount		= 1;

			m_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&combinedMaterialMapsUpdateDesc);

			m_MaterialsResourceDirty = false;
		}
	}
}
