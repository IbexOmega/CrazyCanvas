#include "Rendering/ParticleRenderer.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/ParticleManager.h"

#include "Rendering/RenderAPI.h"
namespace LambdaEngine
{
	ParticleRenderer* ParticleRenderer::s_pInstance = nullptr;

	ParticleRenderer::ParticleRenderer()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;

		m_ParticleCount = 0;
		m_EmitterCount = 1;
	}

	ParticleRenderer::~ParticleRenderer()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;
		if (m_Initilized)
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

	bool LambdaEngine::ParticleRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc perFrameBufferBindingDesc = {};
		perFrameBufferBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferBindingDesc.DescriptorCount = 1;
		perFrameBufferBindingDesc.Binding = 0;
		perFrameBufferBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc textureBindingDesc = {};
		textureBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		textureBindingDesc.DescriptorCount = 100;
		textureBindingDesc.Binding = 0;
		textureBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		textureBindingDesc.Flags = FDescriptorSetLayoutBindingFlag::DESCRIPTOR_SET_LAYOUT_BINDING_FLAG_PARTIALLY_BOUND;

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

		DescriptorBindingDesc emitterBindingDesc = {};
		emitterBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		emitterBindingDesc.DescriptorCount = 1;
		emitterBindingDesc.Binding = 2;
		emitterBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc emitterIndexBindingDesc = {};
		emitterIndexBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		emitterIndexBindingDesc.DescriptorCount = 1;
		emitterIndexBindingDesc.Binding = 3;
		emitterIndexBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorBindingDesc atlasDataBindingDesc = {};
		atlasDataBindingDesc.DescriptorType = EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		atlasDataBindingDesc.DescriptorCount = 1;
		atlasDataBindingDesc.Binding = 0;
		atlasDataBindingDesc.ShaderStageMask = FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings = { perFrameBufferBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings = { textureBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc3 = {};
		descriptorSetLayoutDesc3.DescriptorBindings = { verticesBindingDesc, instanceBindingDesc, emitterBindingDesc, emitterIndexBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc4 = {};
		descriptorSetLayoutDesc4.DescriptorBindings = { atlasDataBindingDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Particle Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc1, descriptorSetLayoutDesc2, descriptorSetLayoutDesc3, descriptorSetLayoutDesc4 };

		m_PipelineLayout = RenderAPI::GetDevice()->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool LambdaEngine::ParticleRenderer::CreateDescriptorSets()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount = 0;
		descriptorCountDesc.TextureDescriptorCount = 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount = 1;
		descriptorCountDesc.ConstantBufferDescriptorCount = 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount = 5;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount = 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount = 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName = "Particle Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount = 64;
		descriptorHeapDesc.DescriptorCount = descriptorCountDesc;

		m_DescriptorHeap = RenderAPI::GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_PerFrameBufferDescriptorSet = RenderAPI::GetDevice()->CreateDescriptorSet("Particle Buffer Descriptor Set 0", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
		if (m_PerFrameBufferDescriptorSet == nullptr)
		{
			LOG_ERROR("[ParticleRenderer]: Failed to create PerFrameBuffer Descriptor Set 0");
			return false;
		}

		return true;
	}

	bool LambdaEngine::ParticleRenderer::CreateShaders()
	{
		bool success = true;

		if (m_MeshShaders)
		{
			m_MeshShaderGUID = ResourceManager::LoadShaderFromFile("/Particles/Particle.mesh", FShaderStageFlag::SHADER_STAGE_FLAG_MESH_SHADER, EShaderLang::SHADER_LANG_GLSL);
			success &= m_MeshShaderGUID != GUID_NONE;
		}
		else
		{
			m_VertexShaderGUID = ResourceManager::LoadShaderFromFile("/Particles/Particle.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
			success &= m_VertexShaderGUID != GUID_NONE;
		}

		m_PixelShaderGUID = ResourceManager::LoadShaderFromFile("/Particles/Particle.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		success &= m_PixelShaderGUID != GUID_NONE;

		return success;
	}

	bool LambdaEngine::ParticleRenderer::CreateCommandLists()
	{
		m_ppGraphicCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppGraphicCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppGraphicCommandAllocators[b] = RenderAPI::GetDevice()->CreateCommandAllocator("Particle Renderer Graphics Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppGraphicCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName = "Particle Renderer Graphics Command List " + std::to_string(b);
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

	bool LambdaEngine::ParticleRenderer::CreateRenderPass(RenderPassAttachmentDesc* pColorAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format = EFormat::FORMAT_R8G8B8A8_UNORM;
		colorAttachmentDesc.SampleCount = 1;
		colorAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_CLEAR;
		colorAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState = pColorAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState = pColorAttachmentDesc->FinalState;

		RenderPassAttachmentDesc depthAttachmentDesc = {};
		depthAttachmentDesc.Format = EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthAttachmentDesc.SampleCount = 1;
		depthAttachmentDesc.LoadOp = ELoadOp::LOAD_OP_LOAD;
		depthAttachmentDesc.StoreOp = EStoreOp::STORE_OP_STORE;
		depthAttachmentDesc.StencilLoadOp = ELoadOp::LOAD_OP_DONT_CARE;
		depthAttachmentDesc.StencilStoreOp = EStoreOp::STORE_OP_DONT_CARE;
		depthAttachmentDesc.InitialState = pDepthStencilAttachmentDesc->InitialState;
		depthAttachmentDesc.FinalState = pDepthStencilAttachmentDesc->FinalState;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass = EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass = 0;
		subpassDependencyDesc.SrcAccessMask = 0;
		subpassDependencyDesc.DstAccessMask = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask = FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName = "Particle Renderer Render Pass";
		renderPassDesc.Attachments = { colorAttachmentDesc, depthAttachmentDesc };
		renderPassDesc.Subpasses = { subpassDesc };
		renderPassDesc.SubpassDependencies = { subpassDependencyDesc };

		m_RenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool LambdaEngine::ParticleRenderer::CreatePipelineState()
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName = "Particle Pipeline State";
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
				.BlendOp					= EBlendOp::BLEND_OP_ADD,
				.SrcBlend					= EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				.DstBlend					= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				.BlendOpAlpha				= EBlendOp::BLEND_OP_ADD,
				.SrcBlendAlpha				= EBlendFactor::BLEND_FACTOR_SRC_ALPHA,
				.DstBlendAlpha				= EBlendFactor::BLEND_FACTOR_INV_SRC_ALPHA,
				.RenderTargetComponentMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A,
				.BlendEnabled				= true
			}
		};

		pipelineStateDesc.VertexShader.ShaderGUID = m_VertexShaderGUID;
		pipelineStateDesc.PixelShader.ShaderGUID = m_PixelShaderGUID;

		m_PipelineStateID = PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
		return true;
	}

	bool LambdaEngine::ParticleRenderer::Init()
	{
		m_BackBufferCount = BACK_BUFFER_COUNT;
		m_ParticleCount = 0;

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[ParticleRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSets())
		{
			LOG_ERROR("[ParticleRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[ParticleRenderer]: Failed to create Shaders");
			return false;
		}

		return true;
	}

	void ParticleRenderer::SetAtlasTexturs(TArray<TextureView*>& textureViews, TArray<Sampler*>& samplers)
	{
		VALIDATE(textureViews.GetSize() == samplers.GetSize());

		m_AtlasCount = textureViews.GetSize();
		m_ppAtlasTextureViews = textureViews.GetData();
		m_ppAtlasSamplers = samplers.GetData();
	}

	bool LambdaEngine::ParticleRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->pColorAttachmentDesc != nullptr);
		VALIDATE(pPreInitDesc->pDepthStencilAttachmentDesc != nullptr);

		if (!m_Initilized)
		{
			if (!CreateCommandLists())
			{
				LOG_ERROR("[ParticleRenderer]: Failed to create render command lists");
				return false;
			}

			if (!CreateRenderPass(pPreInitDesc->pColorAttachmentDesc, pPreInitDesc->pDepthStencilAttachmentDesc))
			{
				LOG_ERROR("[ParticleRenderer]: Failed to create RenderPass");
				return false;
			}

			if (!CreatePipelineState())
			{
				LOG_ERROR("[ParticleRenderer]: Failed to create PipelineState");
				return false;
			}

			// Create a initial particle atlas descriptor set and set the content to a default texture.
			m_AtlasTexturesDescriptorSet = m_DescriptorCache.GetDescriptorSet("Particle Atlas texture Descriptor Set", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
			if (m_AtlasTexturesDescriptorSet != nullptr)
			{
				TextureView* textureView = ResourceManager::GetTextureView(GUID_TEXTURE_DEFAULT_COLOR_MAP);
				Sampler* sampler = Sampler::GetLinearSampler();
				m_AtlasTexturesDescriptorSet->WriteTextureDescriptors(
					&textureView,
					&sampler,
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					0,
					1,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					true
				);
			}
			else
			{
				LOG_ERROR("[ParticleRenderer]: Failed to update DescriptorSet[%d]", 0);
			}

			m_Initilized = true;
		}

		return true;
	}

	void ParticleRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void ParticleRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void ParticleRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(backBufferIndex);

		m_DescriptorCache.HandleUnavailableDescriptors(modFrameIndex);

		// Update m_AtlasTexturesDescriptorSet if new atlases have arrived.
		if (m_PreAtlasCount != m_AtlasCount)
		{
			m_PreAtlasCount = m_AtlasCount;

			constexpr uint32 setIndex = 1U;
			constexpr uint32 setBinding = 0U;

			m_AtlasTexturesDescriptorSet = m_DescriptorCache.GetDescriptorSet("Particle Atlas texture Descriptor Set", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_AtlasTexturesDescriptorSet != nullptr)
			{
				m_AtlasTexturesDescriptorSet->WriteTextureDescriptors(
					m_ppAtlasTextureViews,
					m_ppAtlasSamplers,
					ETextureState::TEXTURE_STATE_SHADER_READ_ONLY,
					setBinding,
					m_AtlasCount,
					EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER,
					true
				);
			}
			else
			{
				LOG_ERROR("[ParticleRenderer]: Failed to update m_AtlasTexturesDescriptorSet");
			}
		}
	}

	void ParticleRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(subImageCount);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == "PARTICLE_IMAGE")
		{
			if (imageCount == 1)
			{
				m_RenderTarget = MakeSharedRef(ppPerImageTextureViews[0]);
			}
			else
			{
				LOG_ERROR("[ParticleRenderer]: Failed to update Render Target Resource");
			}
		}

		if (resourceName == "G_BUFFER_DEPTH_STENCIL")
		{
			if (imageCount == 1)
			{
				m_DepthStencil = MakeSharedRef(ppPerImageTextureViews[0]);
			}
			else
			{
				LOG_ERROR("[ParticleRenderer]: Failed to update Depth Stencil Resource");
			}
		}
	}

	void ParticleRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == PER_FRAME_BUFFER)
		{
			constexpr uint32 setIndex = 0U;
			constexpr uint32 setBinding = 0U;

			m_PerFrameBufferDescriptorSet = m_DescriptorCache.GetDescriptorSet("Per Frame Buffer Descriptor Set 0 Binding 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_PerFrameBufferDescriptorSet != nullptr)
			{
				m_PerFrameBufferDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					setBinding,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[ParticleRenderer]: Failed to update m_PerFrameBufferDescriptorSet");
			}
		}

		if (resourceName == SCENE_PARTICLE_VERTEX_BUFFER)
		{
			if (count == 1)
			{
				constexpr uint32 setIndex = 2U;
				constexpr uint32 setBinding = 0U;
				
				m_VertexInstanceDescriptorSet = m_DescriptorCache.GetDescriptorSet("Vertex Instance Buffer Descriptor Set 2 Binding 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

				if (m_VertexInstanceDescriptorSet != nullptr)
				{
					m_VertexInstanceDescriptorSet->WriteBufferDescriptors(ppBuffers, pOffsets, pSizesInBytes, setBinding, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
				}
				else
				{
					LOG_ERROR("[ParticleRenderer]: Failed to update VertexInstanceDescriptorSet");
				}
			}
		}
		else if (resourceName == SCENE_PARTICLE_INSTANCE_BUFFER)
		{
			if (count == 1)
			{
				constexpr uint32 setIndex = 2U;
				constexpr uint32 setBinding = 1U;

				m_VertexInstanceDescriptorSet = m_DescriptorCache.GetDescriptorSet("Particle Instance Buffer Descriptor Set 2 Binding 1", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

				if (m_VertexInstanceDescriptorSet != nullptr)
				{
					m_VertexInstanceDescriptorSet->WriteBufferDescriptors(ppBuffers, pOffsets, pSizesInBytes, setBinding, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
				}
				else
				{
					LOG_ERROR("[ParticleRenderer]: Failed to update VertexInstanceDescriptorSet");
				}
			}
		}
		else if (resourceName == SCENE_EMITTER_INSTANCE_BUFFER)
		{
			constexpr uint32 setIndex = 2U;
			constexpr uint32 setBinding = 2U;

			m_VertexInstanceDescriptorSet = m_DescriptorCache.GetDescriptorSet("Emitter Instance Buffer Descriptor Set 2 Binding 2", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_VertexInstanceDescriptorSet != nullptr)
			{
				m_VertexInstanceDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					setBinding,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
			}
		}
		else if (resourceName == SCENE_EMITTER_INDEX_BUFFER)
		{
			constexpr uint32 setIndex = 2U;
			constexpr uint32 setBinding = 3U;

			m_VertexInstanceDescriptorSet = m_DescriptorCache.GetDescriptorSet("Emitter Index Buffer Descriptor Set 2 Binding 3", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());
			if (m_VertexInstanceDescriptorSet != nullptr)
			{
				m_VertexInstanceDescriptorSet->WriteBufferDescriptors(
					ppBuffers,
					pOffsets,
					pSizesInBytes,
					setBinding,
					count,
					EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER
				);
			}
			else
			{
				LOG_ERROR("[ParticleUpdater]: Failed to update DescriptorSet[%d]", 0);
			}
		}

		if (resourceName == SCENE_PARTICLE_INDEX_BUFFER)
		{
			if (count == 1)
			{
				m_pIndexBuffer = ppBuffers[0];
			}
		}

		if (resourceName == SCENE_PARTICLE_INDIRECT_BUFFER)
		{
			if (count == 1)
			{
				m_pIndirectBuffer = ppBuffers[0];
			}
		}

		if (resourceName == SCENE_PARTICLE_ATLAS_INFO_BUFFER)
		{
			if (count == 1)
			{
				constexpr uint32 setIndex = 3U;
				constexpr uint32 setBinding = 0U;

				m_AtlasInfoBufferDescriptorSet = m_DescriptorCache.GetDescriptorSet("Atlas Info Buffer Descriptor Set 3 Binding 0", m_PipelineLayout.Get(), setIndex, m_DescriptorHeap.Get());

				if (m_AtlasInfoBufferDescriptorSet != nullptr)
				{
					m_AtlasInfoBufferDescriptorSet->WriteBufferDescriptors(ppBuffers, pOffsets, pSizesInBytes, setBinding, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
				}
				else
				{
					LOG_ERROR("[ParticleRenderer]: Failed to update m_AtlasInfoBufferDescriptorSet");
				}
			}
		}
	}

	void ParticleRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void ParticleRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pDrawArgs);
		UNREFERENCED_VARIABLE(count);
	}

	void ParticleRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool Sleeping)
	{
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppFirstExecutionStage);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(Sleeping);

		if (Sleeping)
			return;

		CommandList* pCommandList = m_ppGraphicCommandLists[modFrameIndex];

		m_ppGraphicCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));

		TSharedRef<const TextureView> renderTarget = m_RenderTarget;
		uint32 width = renderTarget->GetDesc().pTexture->GetDesc().Width;
		uint32 height = renderTarget->GetDesc().pTexture->GetDesc().Height;

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

		ClearColorDesc clearColors[1] = {};

		clearColors[0].Color[0] = 0.f;
		clearColors[0].Color[1] = 0.f;
		clearColors[0].Color[2] = 0.f;
		clearColors[0].Color[3] = 0.f;

		pCommandList->BindDescriptorSetGraphics(m_PerFrameBufferDescriptorSet.Get(), m_PipelineLayout.Get(), 0);
		pCommandList->BindDescriptorSetGraphics(m_AtlasTexturesDescriptorSet.Get(), m_PipelineLayout.Get(), 1);
		pCommandList->BindDescriptorSetGraphics(m_VertexInstanceDescriptorSet.Get(), m_PipelineLayout.Get(), 2);
		pCommandList->BindDescriptorSetGraphics(m_AtlasInfoBufferDescriptorSet.Get(), m_PipelineLayout.Get(), 3);
		pCommandList->BindIndexBuffer(m_pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass = m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets = renderTarget.GetAddressOf();
		beginRenderPassDesc.RenderTargetCount = 1;
		beginRenderPassDesc.pDepthStencil = m_DepthStencil.Get();
		beginRenderPassDesc.Width = width;
		beginRenderPassDesc.Height = height;
		beginRenderPassDesc.Flags = FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors = clearColors;
		beginRenderPassDesc.ClearColorCount = 1;
		beginRenderPassDesc.Offset.x = 0;
		beginRenderPassDesc.Offset.y = 0;
		
		pCommandList->BeginRenderPass(&beginRenderPassDesc);

		if(m_EmitterCount > 0)
			pCommandList->DrawIndexedIndirect(m_pIndirectBuffer, 0, m_EmitterCount, sizeof(IndirectData));

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}
}