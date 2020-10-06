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
#include "Rendering/LineRenderer.h"
#include "Rendering/StagingBufferCache.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "ECS/ECSCore.h"

#include "Game/ECS/Components/Physics/Transform.h"
#include "Game/ECS/Components/Rendering/AnimationComponent.h"
#include "Game/ECS/Components/Rendering/MeshComponent.h"
#include "Game/ECS/Components/Rendering/CameraComponent.h"
#include "Game/ECS/Components/Rendering/PointLightComponent.h"
#include "Game/ECS/Components/Rendering/DirectionalLightComponent.h"

#include "GUI/Core/GUIApplication.h"
#include "GUI/Core/GUIRenderer.h"

#include "Engine/EngineConfig.h"

namespace LambdaEngine
{
	RenderSystem RenderSystem::s_Instance;

	constexpr const uint32 TEMP_DRAW_ARG_MASK = UINT32_MAX;

	bool RenderSystem::Init()
	{
		GraphicsDeviceFeatureDesc deviceFeatures;
		RenderAPI::GetDevice()->QueryDeviceFeatures(&deviceFeatures);
		m_RayTracingEnabled		= deviceFeatures.RayTracing && EngineConfig::GetBoolProperty("RayTracingEnabled");
		m_MeshShadersEnabled	= deviceFeatures.MeshShaders && EngineConfig::GetBoolProperty("MeshShadersEnabled");

		// Subscribe on Static Entities & Dynamic Entities
		{
			TransformComponents transformComponents;
			transformComponents.Position.Permissions	= R;
			transformComponents.Scale.Permissions		= R;
			transformComponents.Rotation.Permissions	= R;

			SystemRegistration systemReg = {};
			systemReg.Phase = g_LastPhase;
			systemReg.SubscriberRegistration.EntitySubscriptionRegistrations =
			{
				{
					{
						{ NDA, MeshComponent::Type() }
					},
					{ &transformComponents },
					{ AnimationComponent::Type() },
					&m_RenderableEntities,
					std::bind(&RenderSystem::OnEntityAdded, this, std::placeholders::_1), 
					std::bind(&RenderSystem::OnEntityRemoved, this, std::placeholders::_1)
				},
				{
					{
						{ R, DirectionalLightComponent::Type() }, 
						{ R, RotationComponent::Type() }
					}, 
					&m_DirectionalLightEntities,	
					std::bind(&RenderSystem::OnDirectionalEntityAdded, this, std::placeholders::_1), 
					std::bind(&RenderSystem::OnDirectionalEntityRemoved, this, std::placeholders::_1)
				},
				{
					{
						{ R, PointLightComponent::Type() }, 
						{ R, PositionComponent::Type() }
					}, 
					&m_PointLightEntities,
					std::bind(&RenderSystem::OnPointLightEntityAdded, this, std::placeholders::_1), 
					std::bind(&RenderSystem::OnPointLightEntityRemoved, this, std::placeholders::_1) 
				},
				{
					{
						{ R, ViewProjectionMatricesComponent::Type() }, 
						{ R, CameraComponent::Type() }
					}, 
					{ &transformComponents }, 
					&m_CameraEntities
				},
				{
					{
						{ R, AnimationComponent::Type() }, 
						{ R, MeshComponent::Type() }
					}, 
					{ &transformComponents }, 
					&m_AnimatedEntities, 
					std::bind(&RenderSystem::OnAnimatedEntityAdded, this, std::placeholders::_1), 
					std::bind(&RenderSystem::OnAnimatedEntityRemoved, this, std::placeholders::_1) 
				}
			};

			RegisterSystem(systemReg);
		}

		//Create Swapchain
		{
			SwapChainDesc swapChainDesc = {};
			swapChainDesc.DebugName		= "Renderer Swap Chain";
			swapChainDesc.pWindow		= CommonApplication::Get()->GetActiveWindow().Get();
			swapChainDesc.pQueue		= RenderAPI::GetGraphicsQueue();
			swapChainDesc.Format		= EFormat::FORMAT_B8G8R8A8_UNORM;
			swapChainDesc.Width			= 0;
			swapChainDesc.Height		= 0;
			swapChainDesc.BufferCount	= BACK_BUFFER_COUNT;
			swapChainDesc.SampleCount	= 1;
			swapChainDesc.VerticalSync	= false;

			m_SwapChain = RenderAPI::GetDevice()->CreateSwapChain(&swapChainDesc);
			if (!m_SwapChain)
			{
				LOG_ERROR("[Renderer]: SwapChain is nullptr after initializaiton");
				return false;
			}

			m_ppBackBuffers		= DBG_NEW Texture*[BACK_BUFFER_COUNT];
			m_ppBackBufferViews	= DBG_NEW TextureView*[BACK_BUFFER_COUNT];

			m_FrameIndex++;
			m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);
		}

		//Create RenderGraph
		{
			RenderGraphStructureDesc renderGraphStructure = {};

			String renderGraphName = EngineConfig::GetStringProperty("RenderGraphName");
			if (renderGraphName != "")
			{
				String prefix	= m_RayTracingEnabled ? "RT_" : "";
				String postfix	= m_MeshShadersEnabled ? "_MESH" : "";
				size_t pos		= renderGraphName.find_first_of(".lrg");
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

			if (!RenderGraphSerializer::LoadAndParse(&renderGraphStructure, renderGraphName, IMGUI_ENABLED))
			{
				LOG_ERROR("[RenderSystem]: Failed to Load RenderGraph, loading Default...");

				renderGraphStructure = {};
				RenderGraphSerializer::LoadAndParse(&renderGraphStructure, "", true);
			}

			RenderGraphDesc renderGraphDesc = {};
			renderGraphDesc.Name						= "Default Rendergraph";
			renderGraphDesc.pRenderGraphStructureDesc	= &renderGraphStructure;
			renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;
			renderGraphDesc.CustomRenderers				= { };

			if (EngineConfig::GetBoolProperty("EnableLineRenderer"))
			{
				m_pLineRenderer = DBG_NEW LineRenderer();
				m_pLineRenderer->init(RenderAPI::GetDevice(), MEGA_BYTE(1), BACK_BUFFER_COUNT);

				renderGraphDesc.CustomRenderers.PushBack(m_pLineRenderer);
			}

			// Light Renderer
			{
				m_LightsDirty = true; // Initilize Light buffer to avoid validation layer errors
				m_pLightRenderer = DBG_NEW LightRenderer();
				m_pLightRenderer->init();

				renderGraphDesc.CustomRenderers.PushBack(m_pLightRenderer);
			}


			//GUI Renderer
			{
				ICustomRenderer* pGUIRenderer = GUIApplication::GetRenderer();
				renderGraphDesc.CustomRenderers.PushBack(pGUIRenderer);
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
				m_ppBackBuffers[v]		= m_SwapChain->GetBuffer(v);
				m_ppBackBufferViews[v]	= m_SwapChain->GetBufferView(v);
			}

			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName							= RENDER_GRAPH_BACK_BUFFER_ATTACHMENT;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextures		= m_ppBackBuffers;
			resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_ppBackBufferViews;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
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
		}

		
		UpdateBuffers();
		UpdateRenderGraph();

		return true;
	}

	bool RenderSystem::Release()
	{
		for (auto& meshAndInstancesIt : m_MeshAndInstancesMap)
		{
			SAFERELEASE(meshAndInstancesIt.second.pBLAS);
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
			SAFERELEASE(meshAndInstancesIt.second.pASInstanceBuffer);

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				SAFERELEASE(meshAndInstancesIt.second.ppASInstanceStagingBuffers[b]);
				SAFERELEASE(meshAndInstancesIt.second.ppRasterInstanceStagingBuffers[b]);
			}
		}

		SAFEDELETE(m_pLineRenderer); 
		SAFEDELETE(m_pLightRenderer);

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

		SAFERELEASE(m_pTLAS);
		SAFERELEASE(m_pCompleteInstanceBuffer);

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
			SAFERELEASE(m_ppStaticStagingInstanceBuffers[b]);
			SAFERELEASE(m_ppLightsStagingBuffer[b]);
		}

		SAFERELEASE(m_pMaterialParametersBuffer);
		SAFERELEASE(m_pPerFrameBuffer);
		SAFERELEASE(m_pLightsBuffer);

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
			const auto& pointLight 	= pPointLightComponents->GetData(entity);
			const auto& position 	= pPositionComponents->GetData(entity);
			if (pointLight.Dirty || position.Dirty)
			{
				UpdatePointLight(entity, position.Position, pointLight.ColorIntensity, pointLight.NearPlane, pointLight.FarPlane);
			}
		}

		ComponentArray<DirectionalLightComponent>* pDirLightComponents = pECSCore->GetComponentArray<DirectionalLightComponent>();
		for (Entity entity : m_DirectionalLightEntities.GetIDs())
		{
			const auto& dirLight = pDirLightComponents->GetData(entity);
			const auto& position = pPositionComponents->GetData(entity);
			const auto& rotation = pRotationComponents->GetData(entity);
			if (dirLight.Dirty || rotation.Dirty || position.Dirty)
			{
				UpdateDirectionalLight(
					dirLight.ColorIntensity,
					position.Position,
					rotation.Quaternion,
					dirLight.frustumWidth,
					dirLight.frustumHeight,
					dirLight.frustumZNear,
					dirLight.frustumZFar
				);
			}
		}

		const ComponentArray<CameraComponent>*					pCameraComponents 	= pECSCore->GetComponentArray<CameraComponent>();
		const ComponentArray<ViewProjectionMatricesComponent>* 	pViewProjComponents	= pECSCore->GetComponentArray<ViewProjectionMatricesComponent>();
		for (Entity entity : m_CameraEntities.GetIDs())
		{
			const auto& cameraComp = pCameraComponents->GetData(entity);
			if (cameraComp.IsActive)
			{
				const auto& positionComp = pPositionComponents->GetData(entity);
				const auto& rotationComp = pRotationComponents->GetData(entity);
				const auto& viewProjComp = pViewProjComponents->GetData(entity);
				UpdateCamera(positionComp.Position, rotationComp.Quaternion, cameraComp, viewProjComp);
			}
		}

		ComponentArray<MeshComponent>*		pMeshComponents			= pECSCore->GetComponentArray<MeshComponent>();
		ComponentArray<AnimationComponent>*	pAnimationComponents	= pECSCore->GetComponentArray<AnimationComponent>();
		for (Entity entity : m_AnimatedEntities)
		{
			MeshComponent&		meshComp		= pMeshComponents->GetData(entity);
			AnimationComponent&	animationComp	= pAnimationComponents->GetData(entity);
			const auto&			positionComp	= pPositionComponents->GetData(entity);
			const auto&			rotationComp	= pRotationComponents->GetData(entity);
			const auto&			scaleComp		= pScaleComponents->GetData(entity);
			
			if (!animationComp.IsPaused)
			{
				MeshKey key(meshComp.MeshGUID, entity, true);
				
				auto meshEntryIt = m_MeshAndInstancesMap.find(key);
				if (meshEntryIt != m_MeshAndInstancesMap.end())
				{
					UpdateAnimationBuffers(animationComp, meshEntryIt->second);
					
					MeshEntry* pMeshEntry = &meshEntryIt->second;
					m_AnimationsToUpdate.insert(pMeshEntry);
					m_DirtyBLASs.insert(pMeshEntry);
					m_TLASDirty = true;
				}
			}

			if (positionComp.Dirty || rotationComp.Dirty || scaleComp.Dirty)
			{
				glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
				transform			*= glm::toMat4(rotationComp.Quaternion);
				transform			= glm::scale(transform, scaleComp.Scale);

				UpdateTransform(entity, transform);
			}
		}

		for (Entity entity : m_RenderableEntities)
		{
			const auto& positionComp	= pPositionComponents->GetData(entity);
			const auto& rotationComp	= pRotationComponents->GetData(entity);
			const auto& scaleComp		= pScaleComponents->GetData(entity);

			if (positionComp.Dirty || rotationComp.Dirty || scaleComp.Dirty)
			{
				glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
				transform *= glm::toMat4(rotationComp.Quaternion);
				transform = glm::scale(transform, scaleComp.Scale);

				UpdateTransform(entity, transform);
			}
		}
	}

	bool RenderSystem::Render()
	{
		m_BackBufferIndex = uint32(m_SwapChain->GetCurrentBackBufferIndex());

		m_FrameIndex++;
		m_ModFrameIndex = m_FrameIndex % uint64(BACK_BUFFER_COUNT);

		StagingBufferCache::Tick();
		CleanBuffers();

		UpdateBuffers();
		UpdateRenderGraph();

		m_pRenderGraph->Update();

		m_pRenderGraph->Render(m_ModFrameIndex, m_BackBufferIndex);

		m_SwapChain->Present();

		return true;
	}

	void RenderSystem::SetRenderGraph(const String& name, RenderGraphStructureDesc* pRenderGraphStructureDesc)
	{
		RenderGraphDesc renderGraphDesc = {};
		renderGraphDesc.Name						= name;
		renderGraphDesc.pRenderGraphStructureDesc	= pRenderGraphStructureDesc;
		renderGraphDesc.BackBufferCount				= BACK_BUFFER_COUNT;

		if (EngineConfig::GetBoolProperty("EnableLineRenderer"))
		{
			m_pLineRenderer = DBG_NEW LineRenderer();
			m_pLineRenderer->init(RenderAPI::GetDevice(), MEGA_BYTE(1), BACK_BUFFER_COUNT);

			renderGraphDesc.CustomRenderers.PushBack(m_pLineRenderer);
		}

		//GUI Renderer
		{
			ICustomRenderer* pGUIRenderer = GUIApplication::GetRenderer();
			renderGraphDesc.CustomRenderers.PushBack(pGUIRenderer);
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
		m_RenderGraphSBTRecordsDirty		= true;
		m_LightsResourceDirty				= true;

		if (m_RayTracingEnabled)
		{
			m_TLASResourceDirty = true;
		}

		UpdateRenderGraph();
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

	glm::mat4 RenderSystem::CreateEntityTransform(Entity entity)
	{
		ECSCore* pECSCore	= ECSCore::GetInstance();
		auto& positionComp	= pECSCore->GetComponent<PositionComponent>(entity);
		auto& rotationComp	= pECSCore->GetComponent<RotationComponent>(entity);
		auto& scaleComp		= pECSCore->GetComponent<ScaleComponent>(entity);

		glm::mat4 transform	= glm::translate(glm::identity<glm::mat4>(), positionComp.Position);
		transform			= transform * glm::toMat4(rotationComp.Quaternion);
		transform			= glm::scale(transform, scaleComp.Scale);
		return transform;
	}

	void RenderSystem::OnEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity);
		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, false);
	}

	void RenderSystem::OnEntityRemoved(Entity entity)
	{
		RemoveEntityInstance(entity);
	}

	void RenderSystem::OnAnimatedEntityAdded(Entity entity)
	{
		ECSCore* pECSCore = ECSCore::GetInstance();
		auto& meshComp = pECSCore->GetComponent<MeshComponent>(entity);

		glm::mat4 transform = CreateEntityTransform(entity);
		AddEntityInstance(entity, meshComp.MeshGUID, meshComp.MaterialGUID, transform, true);
	}

	void RenderSystem::OnAnimatedEntityRemoved(Entity entity)
	{
		RemoveEntityInstance(entity);
	}

	void RenderSystem::OnDirectionalEntityAdded(Entity entity)
	{
		if (!m_DirectionalExist)
		{
			ECSCore* pECSCore = ECSCore::GetInstance();

			const auto& dirLight = pECSCore->GetComponent<DirectionalLightComponent>(entity);
			const auto& position = pECSCore->GetComponent<PositionComponent>(entity);
			const auto& rotation = pECSCore->GetComponent<RotationComponent>(entity);

			UpdateDirectionalLight(
				dirLight.ColorIntensity,
				position.Position,
				rotation.Quaternion,
				dirLight.frustumWidth,
				dirLight.frustumHeight,
				dirLight.frustumZNear,
				dirLight.frustumZFar
			);

			m_DirectionalExist = true;
		}
		else
		{
			LOG_WARNING("Multiple directional lights not supported!");
		}
	}

	void RenderSystem::OnPointLightEntityAdded(Entity entity)
	{
		const ECSCore* pECSCore = ECSCore::GetInstance();

		const auto& pointLight = pECSCore->GetComponent<PointLightComponent>(entity);
		const auto& position = pECSCore->GetComponent<PositionComponent>(entity);


		uint32 pointLightIndex = m_PointLights.GetSize();
		m_EntityToPointLight[entity] = pointLightIndex;
		m_PointLightToEntity[pointLightIndex] = entity;

		m_PointLights.PushBack(PointLight{.ColorIntensity = pointLight.ColorIntensity, .Position = position.Position});
	
		UpdatePointLight(entity, position.Position, pointLight.ColorIntensity, pointLight.NearPlane, pointLight.FarPlane);
	}

	void RenderSystem::OnDirectionalEntityRemoved(Entity entity)
	{
		UNREFERENCED_VARIABLE(entity);

		m_LightBufferData.DirL_ColorIntensity = glm::vec4(0.f);
		m_DirectionalExist = false;
		m_LightsDirty = true;
	}

	void RenderSystem::OnPointLightEntityRemoved(Entity entity)
	{
		uint32 lastIndex = m_PointLights.GetSize() - 1U;
		uint32 lastEntity = m_PointLightToEntity[lastIndex];
		uint32 currentIndex = m_EntityToPointLight[entity];

		m_PointLights[currentIndex] = m_PointLights[lastIndex];

		m_EntityToPointLight[lastEntity] = currentIndex;
		m_PointLightToEntity[currentIndex] = lastEntity;

		m_PointLightToEntity.erase(lastIndex);
		m_EntityToPointLight.erase(entity);
		m_PointLights.PopBack();

		m_LightsDirty = true;
	}

	void RenderSystem::AddEntityInstance(Entity entity, GUID_Lambda meshGUID, GUID_Lambda materialGUID, const glm::mat4& transform, bool isAnimated)
	{
		//auto& component = ECSCore::GetInstance().GetComponent<StaticMeshComponent>(Entity);

		uint32 materialIndex = UINT32_MAX;
		MeshAndInstancesMap::iterator meshAndInstancesIt;

		MeshKey meshKey;
		meshKey.MeshGUID	= meshGUID;
		meshKey.IsAnimated	= isAnimated;
		meshKey.EntityID	= entity;

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
						vertexWeightBufferDesc.SizeInBytes	= pMesh->VertexBoneData.GetSize() * sizeof(VertexBoneData);

						VALIDATE(pMesh->VertexBoneData.GetSize() == pMesh->Vertices.GetSize());

						Buffer* pVertexWeightStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexWeightBufferDesc);
						VALIDATE(pVertexWeightStagingBuffer != nullptr);

						void* pMappedWeights = pVertexWeightStagingBuffer->Map();
						memcpy(pMappedWeights, pMesh->VertexBoneData.GetData(), vertexWeightBufferDesc.SizeInBytes);
						pVertexWeightStagingBuffer->Unmap();

						vertexWeightBufferDesc.DebugName	= "Vertex Weight Buffer";
						vertexWeightBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
						vertexWeightBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
					
						meshEntry.pVertexWeightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&vertexWeightBufferDesc);
						VALIDATE(meshEntry.pVertexWeightsBuffer != nullptr);

						m_PendingBufferUpdates.PushBack({ pVertexWeightStagingBuffer, 0, meshEntry.pVertexWeightsBuffer, 0, vertexWeightBufferDesc.SizeInBytes });
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pVertexWeightStagingBuffer);
					}

					m_PendingBufferUpdates.PushBack({ pVertexStagingBuffer, 0, meshEntry.pVertexBuffer, 0, vertexBufferDesc.SizeInBytes });
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pVertexStagingBuffer);
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
					m_ResourcesToRemove[m_ModFrameIndex].PushBack(pIndexStagingBuffer);
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
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pMeshletStagingBuffer);
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
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pUniqueIndicesStagingBuffer);
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
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pPrimitiveIndicesStagingBuffer);
					}
				}

				meshAndInstancesIt = m_MeshAndInstancesMap.insert({ meshKey, meshEntry }).first;

				if (m_RayTracingEnabled)
				{
					meshAndInstancesIt->second.ShaderRecord.VertexBufferAddress	= meshEntry.pVertexBuffer->GetDeviceAdress();
					meshAndInstancesIt->second.ShaderRecord.IndexBufferAddress	= meshEntry.pIndexBuffer->GetDeviceAdress();
					m_DirtyBLASs.insert(&meshAndInstancesIt->second);
					m_SBTRecordsDirty = true;
				}
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

		InstanceKey instanceKey = {};
		instanceKey.MeshKey			= meshKey;
		instanceKey.InstanceIndex	= meshAndInstancesIt->second.RasterInstances.GetSize();
		m_EntityIDsToInstanceKey[entity] = instanceKey;

		if (m_RayTracingEnabled)
		{
			AccelerationStructureInstance asInstance = {};
			asInstance.Transform		= glm::transpose(transform);
			asInstance.CustomIndex		= materialIndex;
			asInstance.Mask				= 0xFF;
			asInstance.SBTRecordOffset	= 0;
			asInstance.Flags			= RAY_TRACING_INSTANCE_FLAG_FORCE_OPAQUE;

			meshAndInstancesIt->second.ASInstances.PushBack(asInstance);
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
		}

		Instance instance = {};
		instance.Transform		= transform;
		instance.PrevTransform	= transform;
		instance.MaterialIndex	= materialIndex;
		instance.MeshletCount	= meshAndInstancesIt->second.MeshletCount;
		meshAndInstancesIt->second.RasterInstances.PushBack(instance);

		meshAndInstancesIt->second.EntityIDs.PushBack(entity);

		m_DirtyRasterInstanceBuffers.insert(&meshAndInstancesIt->second);

		// Todo: This needs to come from the Entity in some way
		uint32 drawArgHash = TEMP_DRAW_ARG_MASK;
		if (m_RequiredDrawArgs.count(drawArgHash))
		{
			m_DirtyDrawArgs.insert(drawArgHash);
		}
	}

	void RenderSystem::RemoveEntityInstance(Entity entity)
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
			}
		}

		if (m_RayTracingEnabled)
		{
			TArray<AccelerationStructureInstance>& asInstances = meshAndInstancesIt->second.ASInstances;
			asInstances[instanceIndex] = asInstances.GetBack();
			asInstances.PopBack();
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
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

		// Unload Mesh, Todo: Should we always do this?
		if (meshAndInstancesIt->second.EntityIDs.IsEmpty())
		{
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pBLAS);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pVertexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pIndexBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pRasterInstanceBuffer);
			m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pASInstanceBuffer);

			if (meshAndInstancesIt->second.pAnimatedVertexBuffer)
			{
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pAnimatedVertexBuffer);

				VALIDATE(meshAndInstancesIt->second.pAnimationDescriptorSet);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pAnimationDescriptorSet);

				VALIDATE(meshAndInstancesIt->second.pBoneMatrixBuffer);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pBoneMatrixBuffer);

				VALIDATE(meshAndInstancesIt->second.pVertexWeightsBuffer);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.pVertexWeightsBuffer);
			}

			for (uint32 b = 0; b < BACK_BUFFER_COUNT; b++)
			{
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppASInstanceStagingBuffers[b]);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshAndInstancesIt->second.ppRasterInstanceStagingBuffers[b]);
			}

			m_DirtyDrawArgs = m_RequiredDrawArgs;
			m_SBTRecordsDirty = true;

			auto dirtyASInstanceToRemove = std::find_if(m_DirtyASInstanceBuffers.begin(), m_DirtyASInstanceBuffers.end(), [meshAndInstancesIt](const MeshEntry* pMeshEntry)
				{
					return pMeshEntry == &meshAndInstancesIt->second; 
				});
			auto dirtyRasterInstanceToRemove = std::find_if(m_DirtyRasterInstanceBuffers.begin(), m_DirtyRasterInstanceBuffers.end(), [meshAndInstancesIt](const MeshEntry* pMeshEntry)
				{
					return pMeshEntry == &meshAndInstancesIt->second;
				});
			auto dirtyBLASToRemove = std::find_if(m_DirtyBLASs.begin(), m_DirtyBLASs.end(), [meshAndInstancesIt](const MeshEntry* pMeshEntry)
				{
					return pMeshEntry == &meshAndInstancesIt->second;
				});

			if (dirtyASInstanceToRemove != m_DirtyASInstanceBuffers.end()) 
				m_DirtyASInstanceBuffers.erase(dirtyASInstanceToRemove);
			if (dirtyRasterInstanceToRemove != m_DirtyRasterInstanceBuffers.end()) 
				m_DirtyRasterInstanceBuffers.erase(dirtyRasterInstanceToRemove);
			if (dirtyBLASToRemove != m_DirtyBLASs.end()) 
				m_DirtyBLASs.erase(dirtyBLASToRemove);

			m_MeshAndInstancesMap.erase(meshAndInstancesIt);
		}
	}

	void RenderSystem::UpdateDirectionalLight(const glm::vec4& colorIntensity, const glm::vec3& position, const glm::quat& direction, float frustumWidth, float frustumHeight, float zNear, float zFar)
	{
		m_LightBufferData.DirL_ColorIntensity	= colorIntensity;
		m_LightBufferData.DirL_Direction = -GetForward(direction);

		m_LightBufferData.DirL_ProjViews = glm::ortho(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, zNear, zFar);
		m_LightBufferData.DirL_ProjViews *= glm::lookAt(position, position - m_LightBufferData.DirL_Direction, g_DefaultUp);

		m_pRenderGraph->TriggerRenderStage("DIRL_SHADOWMAP");
		m_LightsDirty = true;
	}

	void RenderSystem::UpdatePointLight(Entity entity, const glm::vec3& position, const glm::vec4& colorIntensity, float nearPlane, float farPlane)
	{
		if (m_EntityToPointLight.find(entity) == m_EntityToPointLight.end())
		{
			LOG_ERROR("Entity non-existing in PointLight map!");
			return;
		}
		uint32 index = m_EntityToPointLight[entity];

		m_PointLights[index].ColorIntensity = colorIntensity;
		m_PointLights[index].Position = position;

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
		m_PointLights[index].FarPlane = farPlane;

		glm::mat4 perspective = glm::perspective(glm::radians(FOV), ASPECT_RATIO, nearPlane, farPlane);
		// Create projection matrices for each face
		for (uint32 p = 0; p < PROJECTIONS; p++)
		{
			m_PointLights[index].ProjViews[p] = perspective;
			m_PointLights[index].ProjViews[p] *= glm::lookAt(position, position + directions[p], defaultUp[p]);
		}

		m_pRenderGraph->TriggerRenderStage("RENDER_STAGE_LIGHT");
		m_LightsDirty = true;
	}

	void RenderSystem::UpdateTransform(Entity entity, const glm::mat4& transform)
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
			AccelerationStructureInstance* pASInstanceToUpdate = &meshAndInstancesIt->second.ASInstances[instanceKeyIt->second.InstanceIndex];
			pASInstanceToUpdate->Transform = glm::transpose(transform);
			m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			m_TLASDirty = true;
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

	void RenderSystem::CleanBuffers()
	{
		// Todo: Better solution for this, save some Staging Buffers maybe so they don't get recreated all the time?
		TArray<DeviceChild*>& resourcesToRemove = m_ResourcesToRemove[m_ModFrameIndex];
		for (DeviceChild* pResource : resourcesToRemove)
		{
			SAFERELEASE(pResource);
		}

		resourcesToRemove.Clear();
	}

	void RenderSystem::CreateDrawArgs(TArray<DrawArg>& drawArgs, uint32 mask) const
	{
		UNREFERENCED_VARIABLE(mask);

		for (auto& meshEntryPair : m_MeshAndInstancesMap)
		{
			// Todo: Check Key (or whatever we end up using)
			DrawArg drawArg = { };

			// Assume animated
			if (meshEntryPair.second.pAnimatedVertexBuffer)
			{
				drawArg.pVertexBuffer = meshEntryPair.second.pAnimatedVertexBuffer;
			}
			else
			{
				drawArg.pVertexBuffer = meshEntryPair.second.pVertexBuffer;
			}

			drawArg.pIndexBuffer	= meshEntryPair.second.pIndexBuffer;
			drawArg.IndexCount		= meshEntryPair.second.IndexCount;
			
			drawArg.pInstanceBuffer	= meshEntryPair.second.pRasterInstanceBuffer;
			drawArg.InstanceCount	= meshEntryPair.second.RasterInstances.GetSize();
			
			drawArg.pMeshletBuffer			= meshEntryPair.second.pMeshlets;
			drawArg.MeshletCount			= meshEntryPair.second.MeshletCount;
			drawArg.pUniqueIndicesBuffer	= meshEntryPair.second.pUniqueIndices;
			drawArg.pPrimitiveIndices		= meshEntryPair.second.pPrimitiveIndices;

			drawArgs.PushBack(drawArg);
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
		}

		//Update Empty MaterialData
		{
			UpdateMaterialPropertiesBuffer(pGraphicsCommandList);
		}

		// Perform mesh skinning
		{
			PerformMeshSkinning(pComputeCommandList);
		}

		//Update Acceleration Structures
		if (m_RayTracingEnabled)
		{
			UpdateShaderRecords();
			BuildBLASs(pComputeCommandList);
			UpdateASInstanceBuffers(pComputeCommandList);
			BuildTLAS(pComputeCommandList);
		}
	}

	void RenderSystem::UpdateAnimationBuffers(AnimationComponent& animationComp, MeshEntry& meshEntry)
	{
		// If needed create new buffers
		const uint64 sizeInBytes = animationComp.BoneMatrices.GetSize() * sizeof(glm::mat4);
		if (animationComp.BoneMatrices.GetSize() > meshEntry.BoneMatrixCount)
		{
			if (meshEntry.pBoneMatrixBuffer)
			{
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshEntry.pStagingMatrixBuffer);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshEntry.pBoneMatrixBuffer);
				m_ResourcesToRemove[m_ModFrameIndex].PushBack(meshEntry.pAnimationDescriptorSet);
			}

			BufferDesc matrixBufferDesc = {};
			matrixBufferDesc.DebugName		= "Matrix Staging Buffer";
			matrixBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
			matrixBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
			matrixBufferDesc.SizeInBytes	= sizeInBytes;

			meshEntry.pStagingMatrixBuffer = RenderAPI::GetDevice()->CreateBuffer(&matrixBufferDesc);

			matrixBufferDesc.DebugName	= "Matrix Buffer";
			matrixBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			matrixBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;

			meshEntry.pBoneMatrixBuffer			= RenderAPI::GetDevice()->CreateBuffer(&matrixBufferDesc);
			meshEntry.BoneMatrixCount			= animationComp.BoneMatrices.GetSize();
			meshEntry.pAnimationDescriptorSet	= RenderAPI::GetDevice()->CreateDescriptorSet("Animation Descriptor Set", m_SkinningPipelineLayout.Get(), 0, m_AnimationDescriptorHeap.Get());
			
			const uint64 offset = 0;
			uint64 size = meshEntry.VertexCount * sizeof(Vertex);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pVertexBuffer,			&offset, &size,			0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pAnimatedVertexBuffer,	&offset, &size,			1, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pBoneMatrixBuffer,		&offset, &sizeInBytes,	2, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
			size = meshEntry.VertexCount * sizeof(VertexBoneData);
			meshEntry.pAnimationDescriptorSet->WriteBufferDescriptors(&meshEntry.pVertexWeightsBuffer, &offset, &size, 3, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
		}

		// Copy data
		void* pMapped = meshEntry.pStagingMatrixBuffer->Map();
		memcpy(pMapped, animationComp.BoneMatrices.GetData(), sizeInBytes);
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

			const uint32 workGroupCount = std::max<uint32>(AlignUp(vertexCount, THREADS_PER_WORKGROUP) / THREADS_PER_WORKGROUP, 1u);
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
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

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
						m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pRasterInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName		= "Raster Instance Buffer";
					bufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER;
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

			Buffer* pStagingBuffer = m_ppStaticStagingInstanceBuffers[m_ModFrameIndex];

			if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Staging Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				m_ppStaticStagingInstanceBuffers[m_ModFrameIndex] = pStagingBuffer;
			}

			void* pMapped = pStagingBuffer->Map();
			memcpy(pMapped, m_MaterialProperties.GetData(), requiredBufferSize);
			pStagingBuffer->Unmap();

			if (m_pMaterialParametersBuffer == nullptr || m_pMaterialParametersBuffer->GetDesc().SizeInBytes < requiredBufferSize)
			{
				if (m_pMaterialParametersBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pMaterialParametersBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Material Properties Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER;
				bufferDesc.SizeInBytes	= requiredBufferSize;

				m_pMaterialParametersBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			pCommandList->CopyBuffer(pStagingBuffer, 0, m_pMaterialParametersBuffer, 0, requiredBufferSize);

			m_MaterialsPropertiesBufferDirty = false;
		}
	}

	void RenderSystem::UpdateShaderRecords()
	{
		if (m_SBTRecordsDirty)
		{
			m_SBTRecords.Clear();

			for (MeshAndInstancesMap::iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
			{
				uint32 shaderRecordOffset = m_SBTRecords.GetSize();

				for (AccelerationStructureInstance& asInstance : meshAndInstancesIt->second.ASInstances)
				{
					asInstance.SBTRecordOffset = shaderRecordOffset;
				}

				m_SBTRecords.PushBack(meshAndInstancesIt->second.ShaderRecord);
				m_DirtyASInstanceBuffers.insert(&meshAndInstancesIt->second);
			}

			m_SBTRecordsDirty = false;
			m_TLASDirty = true;
			m_RenderGraphSBTRecordsDirty = true;
		}
	}

	void RenderSystem::BuildBLASs(CommandList* pCommandList)
	{
		if (!m_DirtyBLASs.empty())
		{
			for (MeshEntry* pDirtyBLAS : m_DirtyBLASs)
			{
				//We assume that VertexCount/PrimitiveCount does not change and thus do not check if we need to recreate them

				bool update = true;

				if (pDirtyBLAS->pBLAS == nullptr)
				{
					update = false;

					AccelerationStructureDesc blasCreateDesc = {};
					blasCreateDesc.DebugName		= "BLAS";
					blasCreateDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_BOTTOM;
					blasCreateDesc.Flags			= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
					blasCreateDesc.MaxTriangleCount = pDirtyBLAS->IndexCount / 3;
					blasCreateDesc.MaxVertexCount	= pDirtyBLAS->VertexCount;
					blasCreateDesc.AllowsTransform	= false;

					pDirtyBLAS->pBLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&blasCreateDesc);
				}

				BuildBottomLevelAccelerationStructureDesc blasBuildDesc = {};
				blasBuildDesc.pAccelerationStructure	= pDirtyBLAS->pBLAS;
				blasBuildDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				if (pDirtyBLAS->pAnimatedVertexBuffer)
				{
					blasBuildDesc.pVertexBuffer = pDirtyBLAS->pAnimatedVertexBuffer;
				}
				else
				{
					blasBuildDesc.pVertexBuffer = pDirtyBLAS->pVertexBuffer;
				}

				blasBuildDesc.FirstVertexIndex		= 0;
				blasBuildDesc.VertexStride			= sizeof(Vertex);
				blasBuildDesc.pIndexBuffer			= pDirtyBLAS->pIndexBuffer;
				blasBuildDesc.IndexBufferByteOffset	= 0;
				blasBuildDesc.TriangleCount			= pDirtyBLAS->IndexCount / 3;
				blasBuildDesc.pTransformBuffer		= nullptr;
				blasBuildDesc.TransformByteOffset	= 0;
				blasBuildDesc.Update				= update;

				pCommandList->BuildBottomLevelAccelerationStructure(&blasBuildDesc);

				uint64 blasAddress = pDirtyBLAS->pBLAS->GetDeviceAdress();
				for (AccelerationStructureInstance& asInstance : pDirtyBLAS->ASInstances)
				{
					asInstance.AccelerationStructureAddress = blasAddress;
				}

				m_DirtyASInstanceBuffers.insert(pDirtyBLAS);
			}

			//This is required to sync up BLAS building with TLAS building, to make sure that the BLAS is built before the TLAS
			PipelineMemoryBarrierDesc memoryBarrier = {};
			memoryBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			memoryBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			pCommandList->PipelineMemoryBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &memoryBarrier, 1);

			m_DirtyBLASs.clear();
		}
	}

	void RenderSystem::UpdateASInstanceBuffers(CommandList* pCommandList)
	{
		if (!m_DirtyASInstanceBuffers.empty())
		{
			//AS Instances
			for (MeshEntry* pDirtyInstanceBufferEntry : m_DirtyASInstanceBuffers)
			{
				uint32 requiredBufferSize = pDirtyInstanceBufferEntry->ASInstances.GetSize() * sizeof(AccelerationStructureInstance);

				Buffer* pStagingBuffer = pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex];

				if (pStagingBuffer == nullptr || pStagingBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pStagingBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName = "AS Instance Staging Buffer";
					bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
					bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes = requiredBufferSize;

					pStagingBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
					pDirtyInstanceBufferEntry->ppASInstanceStagingBuffers[m_ModFrameIndex] = pStagingBuffer;
				}

				void* pMapped = pStagingBuffer->Map();
				memcpy(pMapped, pDirtyInstanceBufferEntry->ASInstances.GetData(), requiredBufferSize);
				pStagingBuffer->Unmap();

				if (pDirtyInstanceBufferEntry->pASInstanceBuffer == nullptr || pDirtyInstanceBufferEntry->pASInstanceBuffer->GetDesc().SizeInBytes < requiredBufferSize)
				{
					if (pDirtyInstanceBufferEntry->pASInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pDirtyInstanceBufferEntry->pASInstanceBuffer);

					BufferDesc bufferDesc = {};
					bufferDesc.DebugName = "AS Instance Buffer";
					bufferDesc.MemoryType = EMemoryType::MEMORY_TYPE_GPU;
					bufferDesc.Flags = FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_COPY_SRC;
					bufferDesc.SizeInBytes = requiredBufferSize;

					pDirtyInstanceBufferEntry->pASInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
				}

				pCommandList->CopyBuffer(pStagingBuffer, 0, pDirtyInstanceBufferEntry->pASInstanceBuffer, 0, requiredBufferSize);
			}

			PipelineMemoryBarrierDesc memoryBarrier = {};
			memoryBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			memoryBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			pCommandList->PipelineMemoryBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlag::PIPELINE_STAGE_FLAG_ACCELERATION_STRUCTURE_BUILD, &memoryBarrier, 1);

			m_DirtyASInstanceBuffers.clear();
		}
	}

	void RenderSystem::BuildTLAS(CommandList* pCommandList)
	{
		if (m_TLASDirty)
		{
			m_TLASDirty = false;
			m_CompleteInstanceBufferPendingCopies.Clear();

			uint32 newInstanceCount = 0;

			for (MeshAndInstancesMap::const_iterator meshAndInstancesIt = m_MeshAndInstancesMap.begin(); meshAndInstancesIt != m_MeshAndInstancesMap.end(); meshAndInstancesIt++)
			{
				uint32 instanceCount = meshAndInstancesIt->second.ASInstances.GetSize();

				PendingBufferUpdate copyToCompleteInstanceBuffer = {};
				copyToCompleteInstanceBuffer.pSrcBuffer		= meshAndInstancesIt->second.pASInstanceBuffer;
				copyToCompleteInstanceBuffer.SrcOffset		= 0;
				copyToCompleteInstanceBuffer.DstOffset		= newInstanceCount * sizeof(AccelerationStructureInstance);
				copyToCompleteInstanceBuffer.SizeInBytes	= instanceCount * sizeof(AccelerationStructureInstance);
				m_CompleteInstanceBufferPendingCopies.PushBack(copyToCompleteInstanceBuffer);

				newInstanceCount += instanceCount;
			}

			if (newInstanceCount == 0)
				return;

			uint32 requiredCompleteInstancesBufferSize = newInstanceCount * sizeof(AccelerationStructureInstance);

			if (m_pCompleteInstanceBuffer == nullptr || m_pCompleteInstanceBuffer->GetDesc().SizeInBytes < requiredCompleteInstancesBufferSize)
			{
				if (m_pCompleteInstanceBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pCompleteInstanceBuffer);

				BufferDesc bufferDesc = {};
				bufferDesc.DebugName	= "Complete Instance Buffer";
				bufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
				bufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_RAY_TRACING;
				bufferDesc.SizeInBytes	= requiredCompleteInstancesBufferSize;

				m_pCompleteInstanceBuffer = RenderAPI::GetDevice()->CreateBuffer(&bufferDesc);
			}

			for (const PendingBufferUpdate& pendingUpdate : m_CompleteInstanceBufferPendingCopies)
			{
				pCommandList->CopyBuffer(pendingUpdate.pSrcBuffer, pendingUpdate.SrcOffset, m_pCompleteInstanceBuffer, pendingUpdate.DstOffset, pendingUpdate.SizeInBytes);
			}

			if (m_MeshAndInstancesMap.empty())
				return;

			bool update = true;

			//Recreate TLAS completely if oldInstanceCount != newInstanceCount
			if (m_MaxInstances < newInstanceCount)
			{
				if (m_pTLAS != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pTLAS);

				m_MaxInstances = newInstanceCount;

				AccelerationStructureDesc createTLASDesc = {};
				createTLASDesc.DebugName		= "TLAS";
				createTLASDesc.Type				= EAccelerationStructureType::ACCELERATION_STRUCTURE_TYPE_TOP;
				createTLASDesc.Flags			= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				createTLASDesc.InstanceCount	= m_MaxInstances;

				m_pTLAS = RenderAPI::GetDevice()->CreateAccelerationStructure(&createTLASDesc);

				update = false;

				m_TLASResourceDirty = true;
			}

			if (m_pTLAS != nullptr)
			{
				BuildTopLevelAccelerationStructureDesc buildTLASDesc = {};
				buildTLASDesc.pAccelerationStructure	= m_pTLAS;
				buildTLASDesc.Flags						= FAccelerationStructureFlag::ACCELERATION_STRUCTURE_FLAG_ALLOW_UPDATE;
				buildTLASDesc.Update					= update;
				buildTLASDesc.pInstanceBuffer			= m_pCompleteInstanceBuffer;
				buildTLASDesc.InstanceCount				= newInstanceCount;

				pCommandList->BuildTopLevelAccelerationStructure(&buildTLASDesc);
			}
		}
	}

	void RenderSystem::UpdatePointLightTextureResource()
	{
		uint32 pointLightCount = m_PointLights.GetSize();
		if (pointLightCount == m_CubeTextures.GetSize())
			return;

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
			}
		}
		else if (pointLightCount < m_CubeTextures.GetSize())
		{
			uint32 diff =  m_CubeTextures.GetSize() - pointLightCount;

			// Remove Cube Texture Context for removed pointlights
			for (uint32 r = 0; r < diff; r++)
			{
				SAFEDELETE(m_CubeTextures.GetBack()); m_CubeTextures.PopBack();
				SAFEDELETE(m_CubeTextureViews.GetBack()); m_CubeTextures.PopBack();
				for (uint32 f = 0; f < CUBE_FACE_COUNT && !m_CubeSubImageTextureViews.IsEmpty(); f++)
				{
					SAFEDELETE(m_CubeSubImageTextureViews.GetBack()); m_CubeTextures.PopBack();
				}
			}
		}

		TArray<Sampler*> nearestSamplers(pointLightCount, Sampler::GetNearestSampler());
		ResourceUpdateDesc resourceUpdateDesc = {};
		resourceUpdateDesc.ResourceName = SCENE_POINT_SHADOWMAPS;
		resourceUpdateDesc.ExternalTextureUpdate.ppTextures = m_CubeTextures.GetData();
		resourceUpdateDesc.ExternalTextureUpdate.ppTextureViews = m_CubeTextureViews.GetData();
		resourceUpdateDesc.ExternalTextureUpdate.Count = m_CubeTextureViews.GetSize();
		resourceUpdateDesc.ExternalTextureUpdate.ppPerSubImageTextureViews = m_CubeSubImageTextureViews.GetData();
		resourceUpdateDesc.ExternalTextureUpdate.PerImageSubImageTextureViewCount = CUBE_FACE_COUNT;
		resourceUpdateDesc.ExternalTextureUpdate.ppSamplers = nearestSamplers.GetData();

		m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
	}

	void RenderSystem::UpdateLightsBuffer(CommandList* pCommandList)
	{
		// Light Buffer Initilization
		if (m_LightsDirty)
		{
			size_t pointLightCount			= m_PointLights.GetSize();
			size_t dirLightBufferSize		= sizeof(LightBuffer);
			size_t pointLightsBufferSize	= sizeof(PointLight) * pointLightCount;
			size_t lightBufferSize			= dirLightBufferSize + pointLightsBufferSize;

			// Set point light count
			m_LightBufferData.PointLightCount = float32(pointLightCount);

			Buffer* pCurrentStagingBuffer = m_ppLightsStagingBuffer[m_ModFrameIndex];

			if (pCurrentStagingBuffer == nullptr || pCurrentStagingBuffer->GetDesc().SizeInBytes < lightBufferSize)
			{
				if (pCurrentStagingBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(pCurrentStagingBuffer);

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
				if (m_pLightsBuffer != nullptr) m_ResourcesToRemove[m_ModFrameIndex].PushBack(m_pLightsBuffer);

				BufferDesc lightBufferDesc = {};
				lightBufferDesc.DebugName		= "Lights Buffer";
				lightBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
				lightBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
				lightBufferDesc.SizeInBytes		= lightBufferSize;

				m_pLightsBuffer = RenderAPI::GetDevice()->CreateBuffer(&lightBufferDesc);

				m_LightsResourceDirty = true;
			}

			pCommandList->CopyBuffer(pCurrentStagingBuffer, 0, m_pLightsBuffer, 0, lightBufferSize);
			m_LightsDirty = false;
		}
	}

	void RenderSystem::UpdateRenderGraph()
	{
		//Should we check for Draw Args to be removed here?

		if (!m_DirtyDrawArgs.empty())
		{
			for (uint32 drawArgMask : m_DirtyDrawArgs)
			{
				TArray<DrawArg> drawArgs;
				CreateDrawArgs(drawArgs, drawArgMask);

				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_DRAW_ARGS;
				resourceUpdateDesc.ExternalDrawArgsUpdate.DrawArgsMask	= drawArgMask;
				resourceUpdateDesc.ExternalDrawArgsUpdate.pDrawArgs		= drawArgs.GetData();
				resourceUpdateDesc.ExternalDrawArgsUpdate.Count			= drawArgs.GetSize();

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);
			}

			m_DirtyDrawArgs.clear();
		}

		if (m_RenderGraphSBTRecordsDirty)
		{
			if (!m_SBTRecords.IsEmpty())
			{
				m_pRenderGraph->UpdateGlobalSBT(m_SBTRecords);
			}

			m_RenderGraphSBTRecordsDirty = false;
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

		if (m_LightsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc = {};
			resourceUpdateDesc.ResourceName						= SCENE_LIGHTS_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pLightsBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;
			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			UpdatePointLightTextureResource();

			m_LightsResourceDirty = false;
		}

		if (m_MaterialsResourceDirty)
		{
			ResourceUpdateDesc resourceUpdateDesc				= {};
			resourceUpdateDesc.ResourceName						= SCENE_MAT_PARAM_BUFFER;
			resourceUpdateDesc.ExternalBufferUpdate.ppBuffer	= &m_pMaterialParametersBuffer;
			resourceUpdateDesc.ExternalBufferUpdate.Count		= 1;

			m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

			TArray<Sampler*> linearSamplers(m_AlbedoMaps.GetSize(), Sampler::GetLinearSampler());

			ResourceUpdateDesc albedoMapsUpdateDesc = {};
			albedoMapsUpdateDesc.ResourceName							= SCENE_ALBEDO_MAPS;
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_AlbedoMaps.GetData();
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_AlbedoMapViews.GetData();
			albedoMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= linearSamplers.GetData();
			albedoMapsUpdateDesc.ExternalTextureUpdate.Count			= m_AlbedoMaps.GetSize();

			ResourceUpdateDesc normalMapsUpdateDesc = {};
			normalMapsUpdateDesc.ResourceName							= SCENE_NORMAL_MAPS;
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextures		= m_NormalMaps.GetData();
			normalMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews	= m_NormalMapViews.GetData();
			normalMapsUpdateDesc.ExternalTextureUpdate.ppSamplers		= linearSamplers.GetData();
			normalMapsUpdateDesc.ExternalTextureUpdate.Count			= m_NormalMapViews.GetSize();

			ResourceUpdateDesc combinedMaterialMapsUpdateDesc = {};
			combinedMaterialMapsUpdateDesc.ResourceName								= SCENE_COMBINED_MATERIAL_MAPS;
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppTextures			= m_CombinedMaterialMaps.GetData();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppTextureViews		= m_CombinedMaterialMapViews.GetData();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.ppSamplers			= linearSamplers.GetData();
			combinedMaterialMapsUpdateDesc.ExternalTextureUpdate.Count				= m_CombinedMaterialMaps.GetSize();

			m_pRenderGraph->UpdateResource(&albedoMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&normalMapsUpdateDesc);
			m_pRenderGraph->UpdateResource(&combinedMaterialMapsUpdateDesc);

			m_MaterialsResourceDirty = false;
		}

		if (m_RayTracingEnabled)
		{
			if (m_TLASResourceDirty)
			{
				//Create Resource Update for RenderGraph
				ResourceUpdateDesc resourceUpdateDesc					= {};
				resourceUpdateDesc.ResourceName							= SCENE_TLAS;
				resourceUpdateDesc.ExternalAccelerationStructure.pTLAS	= m_pTLAS;

				m_pRenderGraph->UpdateResource(&resourceUpdateDesc);

				m_TLASResourceDirty = false;
			}
		}
	}
}
