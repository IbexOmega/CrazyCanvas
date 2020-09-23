#include "PreCompiled.h"

#include "Rendering/PhysicsRenderer.h"
#include "Rendering/RenderAPI.h"
#include "Rendering/PipelineStateManager.h"
#include "Rendering/RenderGraph.h"

#include "Rendering/Core/API/CommandAllocator.h"
#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/PipelineLayout.h"
#include "Rendering/Core/API/DescriptorHeap.h"
#include "Rendering/Core/API/DescriptorSet.h"
#include "Rendering/Core/API/PipelineState.h"
#include "Rendering/Core/API/CommandQueue.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/Sampler.h"
#include "Rendering/Core/API/Shader.h"
#include "Rendering/Core/API/Buffer.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Resources/ResourceManager.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{

	PhysicsRenderer::PhysicsRenderer(GraphicsDevice* pGraphicsDevice)
	{
		m_pGraphicsDevice = pGraphicsDevice;

		ConsoleCommand cmdTest;
		cmdTest.Init("physics_render_line", true);
		cmdTest.AddDescription("Renders a test line for physics renderer");
		cmdTest.AddArg(Arg::EType::FLOAT);
		cmdTest.AddArg(Arg::EType::FLOAT);
		cmdTest.AddArg(Arg::EType::FLOAT);
		cmdTest.AddArg(Arg::EType::FLOAT);
		cmdTest.AddArg(Arg::EType::FLOAT);
		cmdTest.AddArg(Arg::EType::FLOAT);
		GameConsole::Get().BindCommand(cmdTest, [this](GameConsole::CallbackInput& input)->void
			{
				drawLine({ input.Arguments[0].Value.Float32, input.Arguments[1].Value.Float32, input.Arguments[2].Value.Float32 },
					{ input.Arguments[3].Value.Float32, input.Arguments[4].Value.Float32, input.Arguments[5].Value.Float32 },
					{ 1.0f, 0.0f, 0.0f });
			});
	}

	PhysicsRenderer::~PhysicsRenderer()
	{
		m_Verticies.Clear();
	}

	bool PhysicsRenderer::init(const PhysicsRendererDesc* pDesc)
	{
		VALIDATE(pDesc);

		uint32 backBufferCount = pDesc->BackBufferCount;
		m_BackBuffers.Resize(backBufferCount);

		// TODO: Check if bullet physics is enabled or initialized
		// if (BulletPhysicsIsNotInitialized())
		// {
		// 	LOG_ERROR("[BulletPhysics]: Failed to initialize BulletPhysics");
		// 	return false;
		// }

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateBuffers(pDesc->VertexBufferSize, pDesc->IndexBufferSize))
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create buffers");
			return false;
		}

		// if (!CreateTextures())
		// {
		// 	LOG_ERROR("[PhysicsRenderer]: Failed to create textures");
		// 	return false;
		// }

		// if (!CreateSamplers())
		// {
		// 	LOG_ERROR("[PhysicsRenderer]: Failed to create samplers");
		// 	return false;
		// }

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSet())
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create Shaders");
			return false;
		}

		uint64 offset 	= 0;
		uint64 size 	= pDesc->VertexBufferSize; //sizeof(VertexData);
		m_DescriptorSet->WriteBufferDescriptors(&m_UniformBuffer, &offset, &size, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);

		return true;
	}

	void PhysicsRenderer::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
	{
		// if (m_DebugMode > 0)
		VertexData fromData = {};
		fromData.Position 	= { from.getX(), from.getY(), from.getZ(), 1.0f };
		fromData.Color		= { color.getX(), color.getY(), color.getZ(), 1.0f };
		m_Verticies.PushBack(fromData);

		VertexData toData	= {};
		toData.Position		= { to.getX(), to.getY(), to.getZ(), 1.0f };
		toData.Color		= { color.getX(), color.getY(), color.getZ(), 1.0f };
		m_Verticies.PushBack(toData);
	}

	void PhysicsRenderer::drawLine(const btVector3& from, const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
	{
		VertexData fromData = {};
		fromData.Position 	= { from.getX(), from.getY(), from.getZ(), 1.0f };
		fromData.Color		= { fromColor.getX(), fromColor.getY(), fromColor.getZ(), 1.0f };
		m_Verticies.PushBack(fromData);

		VertexData toData	= {};
		toData.Position		= { to.getX(), to.getY(), to.getZ(), 1.0f };
		toData.Color		= { toColor.getX(), toColor.getY(), toColor.getZ(), 1.0f };
		m_Verticies.PushBack(toData);
	}

	//void PhysicsRenderer::drawSphere(const btVector3& p, btScalar radius, const btVector3& color)
	//{
	//}

	//void PhysicsRenderer::drawTriangle(const btVector3& a, const btVector3& b, const btVector3& c, const btVector3& color, btScalar alpha)
	//{
	//}

	void PhysicsRenderer::drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color)
	{
	}

	void PhysicsRenderer::reportErrorWarning(const char* warningString)
	{
		LOG_WARNING("[Physics Renderer]: %s", warningString);
	}

	void PhysicsRenderer::draw3dText(const btVector3& location, const char* textString)
	{
	}

	// Custom renderer implementaion
	bool PhysicsRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		VALIDATE(pPreInitDesc->ColorAttachmentCount == 1);

		if (!CreateRenderPass(&pPreInitDesc->pColorAttachmentDesc[0], &pPreInitDesc->pDepthStencilAttachmentDesc[0]))
		{
			LOG_ERROR("[Physics Renderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[Physics Renderer]: Failed to create PipelineState");
			return false;
		}
		
		return true;
	}

	void PhysicsRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void PhysicsRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void PhysicsRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const* ppTextureViews, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(backBufferBound);

		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			for (uint32 i = 0; i < count;  i++)
			{
				m_BackBuffers[i] = MakeSharedRef(ppTextureViews[i]);
			}
		}
		// Might be a bit too hard coded
		else if (resourceName == "G_BUFFER_DEPTH_STENCIL")
		{
			m_DepthStencilBuffer = MakeSharedRef(ppTextureViews[0]);
		}
	}

	void PhysicsRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		// TODO: Ask Herman if I done goofed or done good

		if (count == 1 || backBufferBound)
		{
			auto bufferIt = m_BufferResourceNameDescriptorSetsMap.find(resourceName);
			if (bufferIt == m_BufferResourceNameDescriptorSetsMap.end())
			{
				// If resource doesn't exist, create descriptor and write it
				TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_BufferResourceNameDescriptorSetsMap[resourceName];
				if (backBufferBound)
				{
					// If it is backbufferbound then create copies for each backbuffer
					uint32 backBufferCount = m_BackBuffers.GetSize();
					descriptorSets.Resize(backBufferCount);
					for (uint32 b = 0; b < backBufferCount; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Physics Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						descriptorSets[b] = descriptorSet;

						descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
					}
				}
				else
				{
					// Else create as many as requested
					descriptorSets.Resize(count);
					for (uint32 b = 0; b < count; b++)
					{
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Physics Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						descriptorSets[b] = descriptorSet;

						descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
					}
				}
			}
			else
			{
				// Else the resource exists
				TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_BufferResourceNameDescriptorSetsMap[resourceName];
				if (backBufferBound)
				{
					uint32 backBufferCount = m_BackBuffers.GetSize();
					if (descriptorSets.GetSize() == backBufferCount)
					{
						for (uint32 b = 0; b < backBufferCount; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
							descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
						}
					}
					else
					{
						LOG_ERROR("[Physics Renderer]: Backbuffer count does not match the amount of descriptors to update for resource \"%s\"", resourceName.c_str());
					}
					
				}
				else
				{
					if (descriptorSets.GetSize() == count)
					{
						for (uint32 b = 0; b < count; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
							descriptorSet->WriteBufferDescriptors(&ppBuffers[b], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
						}
					}
					else
					{
						LOG_ERROR("[Physics Renderer]: Buffer count changed between calls to UpdateBufferResource for resource \"%s\"", resourceName.c_str());
					}
					
				}
			}
		}
	}

	void PhysicsRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void PhysicsRenderer::Render(CommandAllocator* pGraphicsCommandAllocator,
		CommandList* pGraphicsCommandList,
		CommandAllocator* pComputeCommandAllocator,
		CommandList* pComputeCommandList,
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppPrimaryExecutionStage,
		CommandList** ppSecondaryExecutionStage)
	{
		UNREFERENCED_VARIABLE(pComputeCommandAllocator);
		UNREFERENCED_VARIABLE(pComputeCommandList);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		TSharedRef<const TextureView> backBuffer = m_BackBuffers[backBufferIndex];
		uint32 width	= backBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height	= backBuffer->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc	= {};
		beginRenderPassDesc.pRenderPass			= m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets		= &backBuffer;
		beginRenderPassDesc.RenderTargetCount	= 1;
		beginRenderPassDesc.pDepthStencil		= m_DepthStencilBuffer.Get();
		beginRenderPassDesc.Width				= width;
		beginRenderPassDesc.Height				= height;
		beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors		= nullptr;
		beginRenderPassDesc.ClearColorCount		= 0;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		if (m_Verticies.GetSize() == 0 || m_DebugMode == 0)
		{
			pGraphicsCommandAllocator->Reset();
			pGraphicsCommandList->Begin(nullptr);
			//Begin and End RenderPass to transition Texture State (Lazy)
			pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);
			pGraphicsCommandList->EndRenderPass();

			pGraphicsCommandList->End();

			(*ppPrimaryExecutionStage) = pGraphicsCommandList;
			return;
		}

		pGraphicsCommandAllocator->Reset();
		pGraphicsCommandList->Begin(nullptr);

		// Transfer data to copy buffers then the GPU buffers
		{
			TSharedRef<Buffer> uniformCopyBuffer = m_UniformCopyBuffers[modFrameIndex];

			uint32 uniformBufferSize	= m_Verticies.GetSize() * sizeof(VertexData);
			byte* pUniformMapping		= reinterpret_cast<byte*>(uniformCopyBuffer->Map());

			memcpy(pUniformMapping, m_Verticies.GetData(), uniformBufferSize);

			uniformCopyBuffer->Unmap();
			pGraphicsCommandList->CopyBuffer(uniformCopyBuffer.Get(), 0, m_UniformBuffer.Get(), 0, uniformBufferSize);
		}

		pGraphicsCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float32)width;
		viewport.Height		= (float32)height;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;
		pGraphicsCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width 	= width;
		scissorRect.Height 	= height;
		pGraphicsCommandList->SetScissorRects(&scissorRect, 0, 1);

		pGraphicsCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));

		if(m_BufferResourceNameDescriptorSetsMap.contains(PER_FRAME_BUFFER))
		{
			auto& descriptorSets = m_BufferResourceNameDescriptorSetsMap[PER_FRAME_BUFFER];
			pGraphicsCommandList->BindDescriptorSetGraphics(descriptorSets[0].Get(), m_PipelineLayout.Get(), 0);
		}

		pGraphicsCommandList->BindDescriptorSetGraphics(m_DescriptorSet.Get(), m_PipelineLayout.Get(), 1);

		pGraphicsCommandList->DrawInstanced(m_Verticies.GetSize(), m_Verticies.GetSize(), 0, 0);

		pGraphicsCommandList->EndRenderPass();
		pGraphicsCommandList->End();

		(*ppPrimaryExecutionStage) = pGraphicsCommandList;

		// TODO: When bullet calls the drawLines, a clear on the verticies might be needed
		//m_Verticies.Clear();
	}

	bool PhysicsRenderer::CreateCopyCommandList()
	{
		m_CopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Physics Renderer Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
		if (!m_CopyCommandAllocator)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName			= "Physics Renderer Copy Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_CopyCommandList = m_pGraphicsDevice->CreateCommandList(m_CopyCommandAllocator.Get(), &commandListDesc);

		return m_CopyCommandList != nullptr;
	}

	bool PhysicsRenderer::CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize)
	{
		BufferDesc uniformCopyBufferDesc = {};
		uniformCopyBufferDesc.DebugName		= "Physics Renderer Uniform Copy Buffer";
		uniformCopyBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		uniformCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		uniformCopyBufferDesc.SizeInBytes	= vertexBufferSize;

		uint32 backBufferCount = m_BackBuffers.GetSize();
		// m_VertexCopyBuffers.Resize(backBufferCount);
		m_UniformCopyBuffers.Resize(backBufferCount);
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			// TSharedRef<Buffer> vertexBuffer = m_pGraphicsDevice->CreateBuffer(&vertexCopyBufferDesc);
			TSharedRef<Buffer> uniformBuffer = m_pGraphicsDevice->CreateBuffer(&uniformCopyBufferDesc);
			if (uniformBuffer != nullptr)
			{
				m_UniformCopyBuffers[b] = uniformBuffer;
			}
			else
			{
				return false;
			}
		}

		BufferDesc uniformBufferDesc = {};
		uniformBufferDesc.DebugName		= "Physics Renderer Uniform Buffer";
		uniformBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		uniformBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_UNORDERED_ACCESS_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
		uniformBufferDesc.SizeInBytes	= vertexBufferSize;

		m_UniformBuffer = m_pGraphicsDevice->CreateBuffer(&uniformBufferDesc);
		if (!m_UniformBuffer)
		{
			return false;
		}

		return true;
	}

	bool PhysicsRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc perFrameBufferDesc = {};
		perFrameBufferDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount	= 1;
		perFrameBufferDesc.Binding			= 0;
		perFrameBufferDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		
		DescriptorBindingDesc ssboBindingDesc = {};
		ssboBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		ssboBindingDesc.DescriptorCount	= 1;
		ssboBindingDesc.Binding			= 0;
		ssboBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1 = {};
		descriptorSetLayoutDesc1.DescriptorBindings		= { perFrameBufferDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2 = {};
		descriptorSetLayoutDesc2.DescriptorBindings		= { ssboBindingDesc };

		// ConstantRangeDesc constantRangeVertexDesc = { };
		// constantRangeVertexDesc.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		// constantRangeVertexDesc.SizeInBytes			= 4 * sizeof(float32);
		// constantRangeVertexDesc.OffsetInBytes		= 0;

		// ConstantRangeDesc constantRangePixelDesc = { };
		// constantRangePixelDesc.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		// constantRangePixelDesc.SizeInBytes		= 8 * sizeof(float32) + sizeof(uint32);
		// constantRangePixelDesc.OffsetInBytes	= constantRangeVertexDesc.SizeInBytes;

		// ConstantRangeDesc pConstantRanges[2] = { constantRangeVertexDesc, constantRangePixelDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName			= "Physics Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };
		// pipelineLayoutDesc.ConstantRanges		= { pConstantRanges[0], pConstantRanges[1] };

		m_PipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool PhysicsRenderer::CreateDescriptorSet()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount					= 0;
		descriptorCountDesc.TextureDescriptorCount					= 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 0;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "Physics Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= 64;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_DescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Physics Renderer Descriptor Set", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());

		return m_DescriptorSet != nullptr;
	}

	bool PhysicsRenderer::CreateShaders()
	{
		m_VertexShaderGUID		= ResourceManager::LoadShaderFromFile("PhysicsDebugVertex.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderGUID		= ResourceManager::LoadShaderFromFile("PhysicsDebugPixel.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderGUID != GUID_NONE && m_PixelShaderGUID != GUID_NONE;
	}

	bool PhysicsRenderer::CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc, RenderPassAttachmentDesc* pDepthStencilAttachmentDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= pBackBufferAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState		= pBackBufferAttachmentDesc->FinalState;

		RenderPassAttachmentDesc depthAttachmentDesc = *pDepthStencilAttachmentDesc;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DEPTH_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "Physics Renderer Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc, depthAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool PhysicsRenderer::CreatePipelineState()
	{
		m_PipelineStateID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID);

		THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
		pixelShaderToPipelineStateMap.insert({ m_PixelShaderGUID, m_PipelineStateID });
		m_ShadersIDToPipelineStateIDMap.insert({ m_VertexShaderGUID, pixelShaderToPipelineStateMap });

		return true;
	}

	uint64 PhysicsRenderer::InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader)
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName			= "Physics Renderer Pipeline State";
		pipelineStateDesc.RenderPass		= m_RenderPass;
		pipelineStateDesc.PipelineLayout	= m_PipelineLayout;

		pipelineStateDesc.InputAssembly.PrimitiveTopology		= EPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINE_LIST;

		pipelineStateDesc.RasterizerState.LineWidth		= 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode 	= EPolygonMode::POLYGON_MODE_LINE;
		pipelineStateDesc.RasterizerState.CullMode		= ECullMode::CULL_MODE_NONE;

		pipelineStateDesc.DepthStencilState = {};
		pipelineStateDesc.DepthStencilState.DepthTestEnable = false;

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

		pipelineStateDesc.VertexShader.ShaderGUID	= vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID	= pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}

}