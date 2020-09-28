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
	PhysicsRenderer* PhysicsRenderer::s_pInstance = nullptr;

	PhysicsRenderer::PhysicsRenderer()
	{
		VALIDATE(s_pInstance == nullptr);
		s_pInstance = this;
	}

	PhysicsRenderer::~PhysicsRenderer()
	{
		VALIDATE(s_pInstance != nullptr);
		s_pInstance = nullptr;

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppRenderCommandLists[b]);
			SAFERELEASE(m_ppRenderCommandAllocators[b]);
		}

		SAFEDELETE_ARRAY(m_ppRenderCommandLists);
		SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);

		m_Verticies.Clear();
		for (auto& lineGroup : m_LineGroups)
		{
			lineGroup.second.Clear();
		}
		m_LineGroups.clear();
	}

	bool PhysicsRenderer::init(GraphicsDevice* pGraphicsDevice, uint32 verticiesBufferSize, uint32 backBufferCount)
	{
		m_BackBuffers.Resize(backBufferCount);
		m_BackBufferCount = backBufferCount;

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
				DrawLine({ input.Arguments[0].Value.Float32, input.Arguments[1].Value.Float32, input.Arguments[2].Value.Float32 },
					{ input.Arguments[3].Value.Float32, input.Arguments[4].Value.Float32, input.Arguments[5].Value.Float32 },
					{ 1.0f, 0.0f, 0.0f });
			});

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateBuffers(verticiesBufferSize))
		{
			LOG_ERROR("[PhysicsRenderer]: Failed to create buffers");
			return false;
		}

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
		uint64 size 	= verticiesBufferSize;
		m_DescriptorSet->WriteBufferDescriptors(&m_UniformBuffer, &offset, &size, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);

		// Draw the XYZ axis in the center of the world
		DrawLine({0.0f, 0.0f, 0.0f,}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
		DrawLine({0.0f, 0.0f, 0.0f,}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
		DrawLine({0.0f, 0.0f, 0.0f,}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f});

		return true;
	}

	uint32 PhysicsRenderer::AddLineGroup(const TArray<glm::vec3>& points, const glm::vec3& color)
	{
		uint32 ID = m_LineGroups.size();
		TArray<VertexData>& vertexPoints = m_LineGroups[ID];
		uint32 size = points.GetSize();

		// If points did not contain an equal amount of points, don't use the last point to avoid wrong drawings
		if (size % 2 != 0)
		{
			size--;
			LOG_INFO("[Physics Renderer]: AddLineGroup recived an uneven amount of points. When adding points the number of points should be divideable by two.");
		}

		vertexPoints.Resize(size);
		for (uint32 p = 0; p < size; p++)
		{
			vertexPoints[p].Position 	= glm::vec4(points[p], 1.0f);
			vertexPoints[p].Color		= glm::vec4(color, 1.0f);
		}

		return ID;
	}

	uint32 PhysicsRenderer::UpdateLineGroup(uint32 ID, const TArray<glm::vec3>& points, const glm::vec3& color)
	{
		if (m_LineGroups.contains(ID))
		{
			TArray<VertexData>& vertexPoints = m_LineGroups[ID];
			if (points.GetSize() > vertexPoints.GetSize())
			{
				vertexPoints.Resize(points.GetSize());
			}
			else if (points.GetSize() < vertexPoints.GetSize())
			{
				vertexPoints.Resize(points.GetSize());
			}
			else
			{
				for (uint32 p = 0; p < vertexPoints.GetSize(); p++)
				{
					vertexPoints[p].Position 	= glm::vec4(points[p], 1.0f);
					vertexPoints[p].Color		= glm::vec4(color, 1.0f);
				}
			}
			return ID;
		}
		else
		{
			return AddLineGroup(points, color);
		}
	}

	void PhysicsRenderer::RemoveLineGroup(uint32 ID)
	{
		if (m_LineGroups.contains(ID))
		{
			auto& arr = m_LineGroups[ID];
			arr.Clear();
			m_LineGroups.erase(ID);
		}
		else
		{
			return;
		}
	}

	void PhysicsRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
	{
		// if (m_DebugMode > 0)
		VertexData fromData = {};
		fromData.Position 	= { from.x, from.y, from.z, 1.0f };
		fromData.Color		= { color.x, color.y, color.z, 1.0f };
		m_Verticies.PushBack(fromData);

		VertexData toData	= {};
		toData.Position		= { to.x, to.y, to.z, 1.0f };
		toData.Color		= { color.x, color.y, color.z, 1.0f };
		m_Verticies.PushBack(toData);
	}

	void PhysicsRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& fromColor, const glm::vec3& toColor)
	{
		VertexData fromData = {};
		fromData.Position 	= { from.x, from.y, from.z, 1.0f };
		fromData.Color		= { fromColor.x, fromColor.y, fromColor.z, 1.0f };
		m_Verticies.PushBack(fromData);

		VertexData toData	= {};
		toData.Position		= { to.x, to.y, to.z, 1.0f };
		toData.Color		= { toColor.x, toColor.y, toColor.z, 1.0f };
		m_Verticies.PushBack(toData);
	}

	//void PhysicsRenderer::DrawSphere(const glm::vec3& p, float radius, const glm::vec3& color)
	//{
	//}

	//void PhysicsRenderer::DrawTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const glm::vec3& color, float alpha)
	//{
	//}

	void PhysicsRenderer::DrawContactPoint(const glm::vec3& PointOnB, const glm::vec3& normalOnB, float distance, int lifeTime, const glm::vec3& color)
	{
	}

	// Custom renderer implementaion
	bool PhysicsRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		VALIDATE(pPreInitDesc->ColorAttachmentCount == 1);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateCommandLists())
		{
			LOG_ERROR("[Physics Renderer]: Failed to create render command lists");
			return false;
		}

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

	void PhysicsRenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage)
	{
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

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];

		if (m_LineGroups.size() == 0 && m_Verticies.GetSize() == 0)
		{
			m_ppRenderCommandAllocators[modFrameIndex]->Reset();
			pCommandList->Begin(nullptr);
			//Begin and End RenderPass to transition Texture State (Lazy)
			pCommandList->BeginRenderPass(&beginRenderPassDesc);
			pCommandList->EndRenderPass();

			pCommandList->End();

			(*ppFirstExecutionStage) = pCommandList;
			return;
		}

		m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		// Transfer data to copy buffers then the GPU buffers
		uint32 drawCount = 0;
		{
			TSharedRef<Buffer> uniformCopyBuffer = m_UniformCopyBuffers[modFrameIndex];

			uint32 totalBufferSize 	= 0;
			byte* pUniformMapping	= reinterpret_cast<byte*>(uniformCopyBuffer->Map());

			for (auto& lineGroup : m_LineGroups)
			{
				const uint32 currentBufferSize = lineGroup.second.GetSize() * sizeof(VertexData);
				memcpy(pUniformMapping + totalBufferSize, lineGroup.second.GetData(), currentBufferSize);
				totalBufferSize += currentBufferSize;
				drawCount += lineGroup.second.GetSize();
			}

			if (m_Verticies.GetSize() > 0)
			{
				memcpy(pUniformMapping + totalBufferSize, m_Verticies.GetData(), m_Verticies.GetSize() * sizeof(VertexData));
				totalBufferSize += m_Verticies.GetSize() * sizeof(VertexData);
				drawCount += m_Verticies.GetSize();
			}

			uniformCopyBuffer->Unmap();
			pCommandList->CopyBuffer(uniformCopyBuffer.Get(), 0, m_UniformBuffer.Get(), 0, totalBufferSize);
		}

		pCommandList->BeginRenderPass(&beginRenderPassDesc);

		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float32)width;
		viewport.Height		= -(float32)height;
		viewport.x			= 0.0f;
		viewport.y			= (float32)height;
		pCommandList->SetViewports(&viewport, 0, 1);

		ScissorRect scissorRect = {};
		scissorRect.Width 	= width;
		scissorRect.Height 	= height;
		pCommandList->SetScissorRects(&scissorRect, 0, 1);

		pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateID));

		if(m_BufferResourceNameDescriptorSetsMap.contains(PER_FRAME_BUFFER))
		{
			auto& descriptorSets = m_BufferResourceNameDescriptorSetsMap[PER_FRAME_BUFFER];
			pCommandList->BindDescriptorSetGraphics(descriptorSets[0].Get(), m_PipelineLayout.Get(), 0);
		}

		pCommandList->BindDescriptorSetGraphics(m_DescriptorSet.Get(), m_PipelineLayout.Get(), 1);

		pCommandList->DrawInstanced(drawCount, drawCount, 0, 0);

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;

		// TODO: When bullet calls the drawLines, a clear on the verticies might be needed
		//m_Verticies.Clear();
	}

	PhysicsRenderer* PhysicsRenderer::Get()
	{
		return s_pInstance;
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

	bool PhysicsRenderer::CreateBuffers(uint32 verticiesBufferSize)
	{
		BufferDesc uniformCopyBufferDesc = {};
		uniformCopyBufferDesc.DebugName		= "Physics Renderer Uniform Copy Buffer";
		uniformCopyBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		uniformCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		uniformCopyBufferDesc.SizeInBytes	= verticiesBufferSize;

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
		uniformBufferDesc.SizeInBytes	= verticiesBufferSize;

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

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName			= "Physics Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc1, descriptorSetLayoutDesc2 };

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

	bool PhysicsRenderer::CreateCommandLists()
	{
		m_ppRenderCommandAllocators	= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppRenderCommandLists		= DBG_NEW CommandList*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Physics Renderer Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName			= "Physics Renderer Render Command List " + std::to_string(b);
			commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppRenderCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

			if (!m_ppRenderCommandLists[b])
			{
				return false;
			}
		}

		return true;
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
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

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

		pipelineStateDesc.VertexShader.ShaderGUID	= vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID	= pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}

}