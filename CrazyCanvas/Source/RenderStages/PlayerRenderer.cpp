#include "RenderStages/PlayerRenderer.h"
#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/RenderAPI.h"

#include "Engine/EngineConfig.h"
#include "ECS/ECSCore.h"
#include "ECS/Components/Team/TeamComponent.h"
#include "ECS/Components/Player/Player.h"

#include "Game/ECS/Systems/Rendering/RenderSystem.h"


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

		SAFEDELETE(m_PushConstant.pData);

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

		m_UsingMeshShader = EngineConfig::GetBoolProperty("MeshShadersEnabled");

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

	void PlayerRenderer::PrepareTextureUpdates(const TArray<UpdateData>& textureIndices)
	{
		UNREFERENCED_VARIABLE(textureIndices);

	}

	void PlayerRenderer::PreBuffersDescriptorSetWrite()
	{
		// called before removing
	}

	void PlayerRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void PlayerRenderer::Update(LambdaEngine::Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);

		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);
	}

	void PlayerRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppPerImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);
		UNREFERENCED_VARIABLE(imageCount);


		// ---------------- START: Not sure what it is used for? -------------------
		// Here I attempt to get a referes to the backbuffers to know about its height and width for render()
		if (resourceName == "INTERMEDIATE_OUTPUT_IMAGE") // Ska jag ändra här? men jag ska endast skriva till intermidate. Ok men för render() så räcker det med
		{						// att få buffers från intermediate?
								// så jag behöver spara båda resursena men inte i m_backbuffer då förslagsvis?
								// ok men att spara undan gör genom att använda MakeSharedRef(ppPerImageTextureViews). OK. TACK
			if (imageCount != 0)
			{
				LOG_WARNING("RenderGraph has been altered. imageCount is %d but should be 0", imageCount);
			}
			m_IntermediateOutputImage = MakeSharedRef(ppPerImageTextureViews[0]);
		}

		if (resourceName == "G_BUFFER_DEPTH_STENCIL") // Ska jag ändra här? men jag ska endast skriva till intermidate. Ok men för render() så räcker det med
		{						// att få buffers från intermediate?
								// så jag behöver spara båda resursena men inte i m_backbuffer då förslagsvis?
								// ok men att spara undan gör genom att använda MakeSharedRef(ppPerImageTextureViews). OK. TACK
			for (uint32 i = 0; i < imageCount; i++)
			{
				// Not sure if this the correct textureView
				m_DepthStencil = MakeSharedRef(ppPerImageTextureViews[0]);
			}
		}

	}

	void PlayerRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(resourceName);


		if (resourceName == SCENE_MAT_PARAM_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 0U;

			m_PlayerMaterialDescriptorSet = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_PlayerMaterialDescriptorSet != nullptr)
			{
				m_PlayerMaterialDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					0,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d]", 0);
			}
		}

		if (resourceName == PER_FRAME_BUFFER)
		{
			constexpr DescriptorSetIndex setIndex = 1U;

			m_PerFrameBufferDescriptorSet = m_DescriptorCache.GetDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_PlayerMaterialDescriptorSet != nullptr)
			{
				m_PerFrameBufferDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					1, // unsure
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[PlayerRenderer]: Failed to update DescriptorSet[%d]", 1);

			}

		}


	}

	void PlayerRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void PlayerRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		if (resourceName == SCENE_DRAW_ARGS)
		{
			if (count > 0U && pDrawArgs != nullptr)
			{
				m_pDrawArgs = pDrawArgs;
				m_DrawCount = count;

				constexpr DescriptorSetIndex setIndex = 0U;

				// Prepare Descriptors for later reusage
				for (auto descriptorSet : m_DrawArgsDescriptorSets)
				{
					m_UnavailableDescriptorSets[setIndex].PushBack(std::make_pair(descriptorSet, m_CurrModFrameIndex));
				}
				m_DrawArgsDescriptorSets.Clear();
				m_DrawArgsDescriptorSets.Resize(m_DrawCount);
				
				// ---------------- START: Not sure what it is used for? -------------------
				// m_DrawCount is telling us how many times to draw per drawcall?
				// Not sure why it would be more than 1 ever
				// Because: DrawCount is telling us how many meshes to draw

				m_ViewerId = MAXUINT32;
				for (uint32 d = 0; d < m_DrawCount; d++)
				{
					// Create a new descriptor or use an old descriptor
					//m_DrawArgsDescriptorSets[d] = GetDescriptorSet("Player Renderer Descriptor Set " + std::to_string(d), setIndex);

					if (m_DrawArgsDescriptorSets[d] != nullptr)
					{
						ECSCore* pECSCore = ECSCore::GetInstance();
						ComponentArray<TeamComponent>* pTeamComponents = pECSCore->GetComponentArray<TeamComponent>();
						ComponentArray<PlayerLocalComponent>* pPlayerLocalComponents = pECSCore->GetComponentArray<PlayerLocalComponent>();

						// Since entityids is vector one could think one pDrawArg[d] has several
						// But pDrawArg[d] is refering to one instance of a player
						// and player is one entity with one id.
						for (Entity entity : m_pDrawArgs[d].EntityIDs)
						{
							if (m_ViewerId != MAXUINT32 && pPlayerLocalComponents->HasComponent(entity))
							{
								m_ViewerId = entity;
							}

							if (pTeamComponents->HasComponent(entity))
							{
								TeamComponent teamComp = pTeamComponents->GetConstData(entity);
								m_TeamIds.PushBack(teamComp.TeamIndex);
							}
						}

						// ---------------- START: Not sure what it is used for? -------------------
						// Hunch: data is packeted and saved here for the draw call in render().
						// another hunch: is packeted because gpu/vulkan stuff want things thight and specified with offsets.
						// This might be where I will put the m_TeamsId and m_ViewerId data. But not just keep it in the member variables?

						// only allies

						// So here I bind nessecary data for rendering my allies? OK
						// what is the differnece? 
						// vertexB(per meshtype) 
						// and InstanceB=For each entity that has a certain mesh like a transform.how many instances of this mesh? 

						// Framebuffer with materials (scenceMatparamBuffer) (Open rendergraph and take it an as extern buffer)
						// then I can use resourseName ´== "scenceMatparamBuffer" to get the material
						
						Buffer* ppBuffers[2] = { m_pDrawArgs[d].pVertexBuffer, m_pDrawArgs[d].pInstanceBuffer };
						uint64 pOffsets[2] = { 0, 0 };
						uint64 pSizes[2] = { m_pDrawArgs[d].pVertexBuffer->GetDesc().SizeInBytes, m_pDrawArgs[d].pInstanceBuffer->GetDesc().SizeInBytes };

						// Sent it to GPU
						m_DrawArgsDescriptorSets[d]->WriteBufferDescriptors(
							ppBuffers,
							pOffsets,
							pSizes,
							0,
							2,
							EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
						);
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

		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));
		pCommandList->BindDescriptorSetGraphics(m_PerFrameBufferDescriptorSet.Get(), m_PipelineLayout.Get(), 0);
		pCommandList->BindDescriptorSetGraphics(m_PlayerMaterialDescriptorSet.Get(), m_PipelineLayout.Get(), 0);

		TArray<TSharedRef<const TextureView>> textures = { m_IntermediateOutputImage, m_DepthStencil };

		for (const auto texture : textures) {
			uint32 width = texture->GetDesc().pTexture->GetDesc().Width;
			uint32 height = texture->GetDesc().pTexture->GetDesc().Height;


			BeginRenderPassDesc beginRenderPassDesc = {};
			beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
			beginRenderPassDesc.ppRenderTargets = texture.GetAddressOf();
			beginRenderPassDesc.RenderTargetCount = 1;
			beginRenderPassDesc.Width = width;
			beginRenderPassDesc.Height = height;
			beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
			beginRenderPassDesc.pClearColors = nullptr;
			beginRenderPassDesc.ClearColorCount = 0;
			beginRenderPassDesc.Offset.x = 0;
			beginRenderPassDesc.Offset.y = 0;

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

			// ---------------- START: Not sure what it is used for? -------------------
			// Looking at other CustomRenderStages they are looping on m_DrawCount
			// m_DrawCount -> how many meshes to draw in total
			// one draw arg is all instances of a mesh


			//// Question: all meshes = all characters?
			//// Yes, den har vi satt. Jag includar alla PlayerBaseComponents.

			//if (m_DrawCount != m_TeamIds.GetSize())
			//{
			//	LOG_MESSAGE("Interesting, explain why this is printing.");
			//}

			for (uint32 d = 0; d < m_DrawCount; d++)
			{
				// Why is this if sats needed?
				if (m_TeamIds.GetSize() <= m_DrawCount)
				{
					/*if (m_ViewerId != m_TeamIds[d])
					{
						continue;
					}*/
				}

				const DrawArg& drawArg = m_pDrawArgs[d];

				pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

				for (size_t i = 0; i < m_DrawArgsDescriptorSets.GetSize(); i++)
				{
					auto* descriptorSet = m_DrawArgsDescriptorSets[d].Get();
					pCommandList->BindDescriptorSetGraphics(descriptorSet, m_PipelineLayout.Get(), (uint32)i);
				}

				pCommandList->DrawIndexInstanced(drawArg.IndexCount, drawArg.InstanceCount, 0, 0, 0);
			}
			pCommandList->EndRenderPass();
		}


		// END ---------------------------------------

		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;

		// Clear TeamIds
		m_TeamIds.Clear();
	}

	void PlayerRenderer::HandleUnavailableDescriptors(uint32 modFrameIndex)
	{
		m_CurrModFrameIndex = modFrameIndex;

		// Go through descriptorSet and see if they are still in use
		for (auto& setIndexArray : m_UnavailableDescriptorSets)
		{
			for (auto descriptorSet = setIndexArray.second.begin(); descriptorSet != setIndexArray.second.end();)
			{
				// Move to available list if 3 frames have pasted since Descriptor Set stopped being used
				if (descriptorSet->second == m_CurrModFrameIndex)
				{
					m_AvailableDescriptorSets[setIndexArray.first].PushBack(descriptorSet->first);
					descriptorSet = setIndexArray.second.Erase(descriptorSet);
				}
				else
					descriptorSet++;
			}
		}
	}

	bool PlayerRenderer::CreatePipelineLayout()
	{
		// PerFrameBuffer
		DescriptorBindingDesc perFrameBufferDesc = {};
		perFrameBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount = 1;
		perFrameBufferDesc.Binding = 0;
		perFrameBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// PerFrameBuffer
		DescriptorBindingDesc materialBufferDesc = {};
		materialBufferDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		materialBufferDesc.DescriptorCount = 1;
		materialBufferDesc.Binding = 1;
		materialBufferDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

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

		// denna mappar till binding = 0
		DescriptorSetLayoutDesc descriptorSetLayoutDesc0 = {};
		descriptorSetLayoutDesc0.DescriptorBindings = { perFrameBufferDesc, materialBufferDesc };

		// denna mappar till binding = 1
		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Player Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1 };
		pipelineLayoutDesc.ConstantRanges = { };
		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool PlayerRenderer::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 0;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;		  // framebuffer
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 3; // Vertice, instance and material
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Player Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 64;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		// Breaks here
		m_PerFrameBufferDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Player Renderer Buffer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
		if (m_PerFrameBufferDescriptorSet == nullptr)
		{
			LOG_ERROR("[PlayerRenderer]: Failed to create DescriptorSet[%d]", 0);
			return false;
		}

		m_PlayerMaterialDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Player Renderer Buffer Descriptor Set 1", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
		if (m_PlayerMaterialDescriptorSet == nullptr)
		{
			LOG_ERROR("[PlayerRenderer]: Failed to create DescriptorSet[%d]", 1);
			return false;
		}


		return true;
	}

	bool PlayerRenderer::CreateShaders()
	{
		m_VertexShaderPointGUID = ResourceManager::LoadShaderFromFile("/Geometry/Geom.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderPointGUID = ResourceManager::LoadShaderFromFile("/Geometry/Geom.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
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
		//subpassDesc.RenderTargetStates = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET }; // solved some stuff not sure why
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

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

		m_PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		return true;
	}

	TSharedRef<DescriptorSet> PlayerRenderer::GetDescriptorSet(const String& debugname, uint32 descriptorLayoutIndex)
	{
		TSharedRef<DescriptorSet> ds;

		if (m_AvailableDescriptorSets.find(descriptorLayoutIndex) != m_AvailableDescriptorSets.end() && !m_AvailableDescriptorSets[descriptorLayoutIndex].IsEmpty())
		{
			ds = m_AvailableDescriptorSets[descriptorLayoutIndex].GetBack();
			m_AvailableDescriptorSets[descriptorLayoutIndex].PopBack();
		}
		else
		{
			ds = RenderAPI::GetDevice()->CreateDescriptorSet(debugname, m_PipelineLayout.Get(), descriptorLayoutIndex, m_DescriptorHeap.Get());
			if (ds == nullptr)
			{
				LOG_ERROR("[PlayerRenderer]: Failed to create DescriptorSet[%d]", 1);
				return nullptr;
			}
		}

		return ds;
	}
}