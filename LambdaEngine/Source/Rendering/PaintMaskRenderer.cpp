#include "PreCompiled.h"
#include "Rendering/PaintMaskRenderer.h"

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
#include "Rendering/EntityMaskManager.h"
#include "Game/ECS/Systems/Rendering/RenderSystem.h"

#include "Game/ECS/Components/Rendering/MeshPaintComponent.h"

#include "Application/API/Window.h"
#include "Application/API/CommonApplication.h"

#include "Resources/ResourceManager.h"

#include "Game/GameConsole.h"

namespace LambdaEngine
{
	TArray<PaintMaskRenderer::UnwrapData>	PaintMaskRenderer::s_ServerCollisions;
	TArray<PaintMaskRenderer::UnwrapData>	PaintMaskRenderer::s_ClientCollisions;
	bool									PaintMaskRenderer::s_ShouldReset = false;

	PaintMaskRenderer::PaintMaskRenderer(GraphicsDevice* pGraphicsDevice, uint32 backBufferCount)
	{
		m_BackBuffers.Resize(backBufferCount);
		m_BackBufferCount = backBufferCount;

		m_pDeviceResourcesToDestroy.Resize(m_BackBufferCount);

		m_pGraphicsDevice = pGraphicsDevice;
	}

	PaintMaskRenderer::~PaintMaskRenderer()
	{
		if (m_ppRenderCommandLists && m_ppRenderCommandAllocators)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppRenderCommandLists[b]);
				SAFERELEASE(m_ppRenderCommandAllocators[b]);

				for (TSharedRef<DeviceChild> pDeviceChild : m_pDeviceResourcesToDestroy[b])
				{
					SAFERELEASE(pDeviceChild);
				}
			}

			SAFEDELETE_ARRAY(m_ppRenderCommandLists);
			SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
			m_pDeviceResourcesToDestroy.Clear();
		}
	}

	bool PaintMaskRenderer::Init()
	{
		if (!m_pGraphicsDevice)
		{
			LOG_ERROR("[PaintMaskRenderer]: Graphic Device is null.");
			return false;
		}

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateBuffers())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create buffers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSet())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create Shaders");
			return false;
		}

		ConsoleCommand cmdHitTest;
		cmdHitTest.Init("add_hit_point", true);
		cmdHitTest.AddFlag("p", Arg::EType::FLOAT, 3);
		cmdHitTest.AddFlag("d", Arg::EType::FLOAT, 3);
		cmdHitTest.AddFlag("paint", Arg::EType::INT);
		cmdHitTest.AddDescription("Add a hitpoint for the paint mask renderer\n\t[-p] position of point in world\n\t[-d] direction of point in world\n\t[-paint] true to paint, false to remove paint\n\t'add_hit_point <-p x y z> [-d x y z] [-paint true/false]'");
		GameConsole::Get().BindCommand(cmdHitTest, [&, this](GameConsole::CallbackInput& input)->void {
			if (!input.Flags.contains("p"))
			{
				GameConsole::Get().PushError("-p (position) is required");
				return;
			}
			else if (input.Flags["p"].NumUsedArgs != 3)
			{
				GameConsole::Get().PushError("-p (position) requires three coordinates, but only " + std::to_string(input.Flags["p"].NumUsedArgs) + " were given");
				return;
			}

			glm::vec3 pos = {input.Flags["p"].Args[0].Value.Float32, input.Flags["p"].Args[1].Value.Float32, input.Flags["p"].Args[2].Value.Float32};
			glm::vec3 dir = {1.0f, 0.0f, 0.0f};

			if (input.Flags.contains("d") && input.Flags["d"].NumUsedArgs == 3)
			{
				dir = {input.Flags["d"].Args[0].Value.Float32, input.Flags["d"].Args[1].Value.Float32, input.Flags["d"].Args[2].Value.Float32};
			}
			else
			{
				GameConsole::Get().PushMsg("Direction not given or too few positions for flag", {0.8f, 0.8f, 0.0f, 1.0f});
			}

			EPaintMode paintMode = EPaintMode::PAINT;
			if (input.Flags.contains("paint") && input.Flags["paint"].Arg.Value.Int32 >= 0)
			{
				int32 mode = input.Flags["paint"].Arg.Value.Int32;
				paintMode = mode == 0 ? EPaintMode::REMOVE : EPaintMode::PAINT;
			}

			PaintMaskRenderer::AddHitPoint(pos, dir, paintMode, ERemoteMode::SERVER, ETeam::RED);
			});

		return false;
	}

	bool PaintMaskRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateCommandLists())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create render command lists");
			return false;
		}

		if (!CreateRenderPass(pPreInitDesc))
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[PaintMaskRenderer]: Failed to create PipelineState");
			return false;
		}

		uint64 offset = 0;
		uint64 size = sizeof(UnwrapData) * MAX_PAINT_PER_FRAME;
		Buffer* buffer = m_UnwrapDataBuffer.Get();
		UpdateBufferResource("UNWRAP_DATA_BUFFER", &buffer, &offset, &size, 1, false);

		return true;
	}

	void PaintMaskRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
	}


	void PaintMaskRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	void PaintMaskRenderer::UpdateTextureResource(const String& resourceName, const TextureView* const * ppPerImageTextureViews, const TextureView* const* ppPerSubImageTextureViews, uint32 imageCount, uint32 subImageCount, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(ppPerImageTextureViews);
		UNREFERENCED_VARIABLE(ppPerSubImageTextureViews);
		UNREFERENCED_VARIABLE(subImageCount);

		if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
		{
			for (uint32 i = 0; i < imageCount; i++)
			{
				m_BackBuffers[i] = MakeSharedRef(ppPerImageTextureViews[i]);
			}
		}

		if (resourceName == "BRUSH_MASK")
		{
			VALIDATE(imageCount == 1);
			VALIDATE(backBufferBound == false);

			// This should not be necessary, because we already know that the brush mask texture is not back buffer bound.
			Sampler* sampler = Sampler::GetLinearSampler();
			if (!m_BrushMaskDescriptorSet.Get())
			{
				m_BrushMaskDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Buffer Descriptor Set", m_PipelineLayout.Get(), 1, m_DescriptorHeap.Get());
				m_BrushMaskDescriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[0], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
			else
			{
				m_BrushMaskDescriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[0], &sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
			}
		}
	}

	void PaintMaskRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		if (count == 1 || backBufferBound)
		{
			if (resourceName == PER_FRAME_BUFFER)
			{
				if (!m_PerFrameBufferDescriptorSet.Get())
				{
					m_PerFrameBufferDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom PerFrameBuffer Buffer Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
					m_PerFrameBufferDescriptorSet->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
				}
				else
				{
					if (m_BrushMaskDescriptorSet.Get())
					{
						m_BrushMaskDescriptorSet->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
					}
					else
					{
						LOG_ERROR("[Paint Mask Renderer]: Buffer count changed between calls to UpdateBufferResource for resource \"%s\"", resourceName.c_str());
					}
				}
			}
			else if (resourceName == "UNWRAP_DATA_BUFFER")
			{
				if (!m_UnwrapDataDescriptorSet.Get())
				{
					m_UnwrapDataDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Unwrap Data Buffer Descriptor Set", m_PipelineLayout.Get(), 3, m_DescriptorHeap.Get());
					m_UnwrapDataDescriptorSet->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
				}
				else
				{
					m_UnwrapDataDescriptorSet->WriteBufferDescriptors(&ppBuffers[0], pOffsets, pSizesInBytes, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER);
				}
			}
		}
	}

	void PaintMaskRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void PaintMaskRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		UNREFERENCED_VARIABLE(resourceName);

		m_pDrawArgs = pDrawArgs;

		uint32 backBufferCount = m_BackBuffers.GetSize();
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			if (m_VerticesInstanceDescriptorSets.IsEmpty())
				m_VerticesInstanceDescriptorSets.Resize(backBufferCount);

			// Remove all previous descriptor sets for the vertices and instance data.
			for (TSharedRef<DescriptorSet> descriptorSet : m_VerticesInstanceDescriptorSets[b])
				m_pDeviceResourcesToDestroy[b].PushBack(std::move(descriptorSet));
			m_VerticesInstanceDescriptorSets[b].Clear();

			for (uint32 drawArgIndex = 0; drawArgIndex < count; drawArgIndex++)
			{
				const DrawArg& drawArg = pDrawArgs[drawArgIndex];

				// Create new descriptor sets for the vertices and instances.
				{
					TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("Paint Mask Renderer Custom Vertex Buffer Descriptor Set", m_PipelineLayout.Get(), 2, m_DescriptorHeap.Get());
					uint64 size = drawArg.pVertexBuffer->GetDesc().SizeInBytes;
					uint64 offset = 0;
					descriptorSet->WriteBufferDescriptors(&drawArg.pVertexBuffer, &offset, &size, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
					size = drawArg.pInstanceBuffer->GetDesc().SizeInBytes;
					offset = 0;
					descriptorSet->WriteBufferDescriptors(&drawArg.pInstanceBuffer, &offset, &size, 1, 1, EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER);
					m_VerticesInstanceDescriptorSets[b].PushBack(descriptorSet);
				}
			}
		}

		m_RenderTargets.Clear();
		for (uint32 d = 0; d < count; d++)
		{
			const DrawArg& drawArg = pDrawArgs[d];

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
							m_RenderTargets.PushBack({ .pTextureView = pTextureView, .DrawArgIndex = d, .InstanceIndex = i });
						}
					}
				}
			}
		}
	}

	void PaintMaskRenderer::Render(uint32 modFrameIndex, uint32 backBufferIndex, CommandList** ppFirstExecutionStage, CommandList** ppSecondaryExecutionStage, bool sleeping)
	{
		UNREFERENCED_VARIABLE(backBufferIndex);
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);
		UNREFERENCED_VARIABLE(sleeping);

		// Delete old resources.
		TArray<TSharedRef<DeviceChild>>& currentFrameDeviceResourcesToDestroy = m_pDeviceResourcesToDestroy[modFrameIndex];
		if (!currentFrameDeviceResourcesToDestroy.IsEmpty())
		{
			currentFrameDeviceResourcesToDestroy.Clear();
		}

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];

		if ((m_RenderTargets.IsEmpty() || (s_ClientCollisions.IsEmpty() && s_ServerCollisions.IsEmpty())) && !s_ShouldReset)
		{
			return;
		}

		m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		// "combine" both arrays to loop through
		TArray<TArray<UnwrapData>*> collisions{&s_ClientCollisions, &s_ServerCollisions};

		for (uint32 index = 0; auto& collisionArray : collisions)
		{
			bool isServer = false;

			FrameSettings frameSettings = {};
			frameSettings.ShouldReset = s_ShouldReset;

			// Transfer current collision data
			if (!collisionArray->IsEmpty())
			{
				TSharedRef<Buffer> unwrapDataCopyBuffer = m_UnwrapDataCopyBuffers[modFrameIndex][index++];

				byte* pUniformMapping	= reinterpret_cast<byte*>(unwrapDataCopyBuffer->Map());

				const UnwrapData& data	= collisionArray->GetFront();
				isServer = data.RemoteMode == ERemoteMode::SERVER ? true : false;
				frameSettings.ShouldPaint = data.RemoteMode != ERemoteMode::UNDEFINED && data.PaintMode != EPaintMode::NONE;

				uint32 size = 0;
				// Current limit is 10 draw calls per frame - might change in future if needed
				if (collisionArray->GetSize() > MAX_PAINT_PER_FRAME)
				{
					size = 10;
					memcpy(pUniformMapping, collisionArray->GetData(), sizeof(UnwrapData) * size);

					// Handle the swaperino
					uint32 extraHits = collisionArray->GetSize() - MAX_PAINT_PER_FRAME;
					for (uint32 newIndex = 0, currentIndex = MAX_PAINT_PER_FRAME; currentIndex < collisionArray->GetSize(); currentIndex++)
					{
						(*collisionArray)[newIndex++] = (*collisionArray)[currentIndex];
					}
					collisionArray->Resize(extraHits);

					LOG_INFO("Extra hits: %d", extraHits);
				}
				else
				{
					size = collisionArray->GetSize();
					memcpy(pUniformMapping, collisionArray->GetData(), sizeof(UnwrapData) * size);
					collisionArray->Clear();
				}

				unwrapDataCopyBuffer->Unmap();
				pCommandList->CopyBuffer(unwrapDataCopyBuffer.Get(), 0, m_UnwrapDataBuffer.Get(), 0, sizeof(UnwrapData) * size);
				frameSettings.PaintCount = size;
			}

			if (!frameSettings.ShouldReset && !frameSettings.ShouldPaint)
			{
				continue;
			}

			for (uint32 t = 0; t < m_RenderTargets.GetSize(); t++)
			{
				RenderTarget	renderTargetDesc	= m_RenderTargets[t];
				uint32			drawArgIndex		= renderTargetDesc.DrawArgIndex;
				uint32			instanceIndex		= renderTargetDesc.InstanceIndex;
				const DrawArg&	drawArg				= m_pDrawArgs[drawArgIndex];
				TextureView*	renderTarget		= renderTargetDesc.pTextureView;

				uint32 width	= renderTarget->GetDesc().pTexture->GetDesc().Width;
				uint32 height	= renderTarget->GetDesc().pTexture->GetDesc().Height;

				BeginRenderPassDesc beginRenderPassDesc = {};
				beginRenderPassDesc.pRenderPass			= m_RenderPass.Get();
				beginRenderPassDesc.ppRenderTargets		= &renderTarget;
				beginRenderPassDesc.RenderTargetCount	= 1;
				beginRenderPassDesc.Width				= width;
				beginRenderPassDesc.Height				= height;
				beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
				beginRenderPassDesc.pClearColors		= nullptr;
				beginRenderPassDesc.ClearColorCount		= 0;
				beginRenderPassDesc.Offset.x			= 0;
				beginRenderPassDesc.Offset.y			= 0;

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
				scissorRect.Width	= width;
				scissorRect.Height	= height;
				pCommandList->SetScissorRects(&scissorRect, 0, 1);

				if (isServer && s_ShouldReset)
					pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateBothID));
				else if (isServer)
					pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateServerID));
				else
					pCommandList->BindGraphicsPipeline(PipelineStateManager::GetPipelineState(m_PipelineStateClientID));

				pCommandList->BindIndexBuffer(drawArg.pIndexBuffer, 0, EIndexType::INDEX_TYPE_UINT32);

				if (m_PerFrameBufferDescriptorSet.Get())
				{
					pCommandList->BindDescriptorSetGraphics(m_PerFrameBufferDescriptorSet.Get(), m_PipelineLayout.Get(), 0);
				}

				if (m_BrushMaskDescriptorSet.Get())
				{
					pCommandList->BindDescriptorSetGraphics(m_BrushMaskDescriptorSet.Get(), m_PipelineLayout.Get(), 1);
				}

				if (m_UnwrapDataDescriptorSet.Get())
				{
					pCommandList->BindDescriptorSetGraphics(m_UnwrapDataDescriptorSet.Get(), m_PipelineLayout.Get(), 3);
				}

				pCommandList->BindDescriptorSetGraphics(m_VerticesInstanceDescriptorSets[modFrameIndex][drawArgIndex].Get(), m_PipelineLayout.Get(), 2);

				pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, &instanceIndex, sizeof(uint32), 0);
				pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, &frameSettings, sizeof(FrameSettings), sizeof(uint32));

				pCommandList->DrawIndexInstanced(drawArg.IndexCount, 1, 0, 0, 0);

				pCommandList->EndRenderPass();

				if (renderTarget->GetTexture()->GetDesc().Miplevels > 1)
					pCommandList->GenerateMiplevels(renderTarget->GetTexture(), ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, false);
			}
		}

		s_ShouldReset = false;
		pCommandList->End();
		(*ppFirstExecutionStage) = pCommandList;
	}

	void PaintMaskRenderer::AddHitPoint(const glm::vec3& position, const glm::vec3& direction, EPaintMode paintMode, ERemoteMode remoteMode, ETeam team)
	{
		UnwrapData data = {};
		data.TargetPosition		= { position.x, position.y, position.z, 1.0f };
		data.TargetDirection	= { direction.x, direction.y, direction.z, 1.0f };
		data.PaintMode			= paintMode;
		data.RemoteMode			= remoteMode;
		data.Team				= team;

		if (remoteMode == ERemoteMode::CLIENT)
			s_ClientCollisions.PushBack(data);
		else if (remoteMode == ERemoteMode::SERVER)
			s_ServerCollisions.PushBack(data);
	}

	void PaintMaskRenderer::ResetClient()
	{
		s_ShouldReset = true;
	}

	bool PaintMaskRenderer::CreateCopyCommandList()
	{
		m_CopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("Paint Mask Renderer Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);
		if (!m_CopyCommandAllocator)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName			= "Paint Mask Renderer Copy Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_CopyCommandList = m_pGraphicsDevice->CreateCommandList(m_CopyCommandAllocator.Get(), &commandListDesc);

		return m_CopyCommandList != nullptr;
	}

	bool PaintMaskRenderer::CreateBuffers()
	{
		BufferDesc uniformCopyBufferDesc	= {};
		uniformCopyBufferDesc.DebugName		= "Paint Mask Renderer Unwrap Data Copy Buffer";
		uniformCopyBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		uniformCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		uniformCopyBufferDesc.SizeInBytes	= sizeof(UnwrapData) * MAX_PAINT_PER_FRAME;

		uint32 backBufferCount = m_BackBuffers.GetSize();
		m_UnwrapDataCopyBuffers.Resize(backBufferCount);
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			m_UnwrapDataCopyBuffers[b].Resize(2);
			for (uint32 c = 0; c < 2; c++)
			{
				TSharedRef<Buffer> uniformBuffer = m_pGraphicsDevice->CreateBuffer(&uniformCopyBufferDesc);
				if (uniformBuffer != nullptr)
				{
					m_UnwrapDataCopyBuffers[b][c] = uniformBuffer;
				}
				else
				{
					return false;
				}
			}
		}

		BufferDesc uniformBufferDesc	= {};
		uniformBufferDesc.DebugName		= "Paint Mask Renderer Unwrap Data Buffer";
		uniformBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		uniformBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_CONSTANT_BUFFER | FBufferFlag::BUFFER_FLAG_COPY_DST;
		uniformBufferDesc.SizeInBytes	= uniformCopyBufferDesc.SizeInBytes;

		m_UnwrapDataBuffer = m_pGraphicsDevice->CreateBuffer(&uniformBufferDesc);
		return m_UnwrapDataBuffer != nullptr;
	}

	bool PaintMaskRenderer::CreatePipelineLayout()
	{
		ConstantRangeDesc constantRangeVertexDesc		= { };
		constantRangeVertexDesc.ShaderStageFlags		= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		constantRangeVertexDesc.SizeInBytes				= sizeof(uint32);
		constantRangeVertexDesc.OffsetInBytes			= 0;

		ConstantRangeDesc constantRangePixelDesc		= { };
		constantRangePixelDesc.ShaderStageFlags			= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		constantRangePixelDesc.SizeInBytes				= sizeof(FrameSettings);
		constantRangePixelDesc.OffsetInBytes			= sizeof(uint32);

		// PerFrameBuffer
		DescriptorBindingDesc perFrameBufferDesc		= {};
		perFrameBufferDesc.DescriptorType				= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		perFrameBufferDesc.DescriptorCount				= 1;
		perFrameBufferDesc.Binding						= 0;
		perFrameBufferDesc.ShaderStageMask				= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Brush mask texture
		DescriptorBindingDesc brushMaskDesc				= {};
		brushMaskDesc.DescriptorType					= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		brushMaskDesc.DescriptorCount					= 1;
		brushMaskDesc.Binding							= 0;
		brushMaskDesc.ShaderStageMask					= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Draw Args (No Extension, only Vertices and Instances)
		DescriptorBindingDesc ssboVerticesBindingDesc	= {};
		ssboVerticesBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		ssboVerticesBindingDesc.DescriptorCount			= 1;
		ssboVerticesBindingDesc.Binding					= 0;
		ssboVerticesBindingDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorBindingDesc ssboInstancesBindingDesc	= {};
		ssboInstancesBindingDesc.DescriptorType			= EDescriptorType::DESCRIPTOR_TYPE_UNORDERED_ACCESS_BUFFER;
		ssboInstancesBindingDesc.DescriptorCount		= 1;
		ssboInstancesBindingDesc.Binding				= 1;
		ssboInstancesBindingDesc.ShaderStageMask		= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		// Unwrap shaders data
		DescriptorBindingDesc unwrapDataBufferDesc		= {};
		unwrapDataBufferDesc.DescriptorType				= EDescriptorType::DESCRIPTOR_TYPE_CONSTANT_BUFFER;
		unwrapDataBufferDesc.DescriptorCount			= 1;
		unwrapDataBufferDesc.Binding					= 0;
		unwrapDataBufferDesc.ShaderStageMask			= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER | FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc0	= {};
		descriptorSetLayoutDesc0.DescriptorBindings			= { perFrameBufferDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc1	= {};
		descriptorSetLayoutDesc1.DescriptorBindings			= { brushMaskDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc2	= {};
		descriptorSetLayoutDesc2.DescriptorBindings			= { ssboVerticesBindingDesc, ssboInstancesBindingDesc };

		DescriptorSetLayoutDesc descriptorSetLayoutDesc3	= {};
		descriptorSetLayoutDesc3.DescriptorBindings			= { unwrapDataBufferDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName = "Paint Mask Renderer Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts = { descriptorSetLayoutDesc0, descriptorSetLayoutDesc1, descriptorSetLayoutDesc2, descriptorSetLayoutDesc3 };
		pipelineLayoutDesc.ConstantRanges = { constantRangeVertexDesc, constantRangePixelDesc };

		m_PipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool PaintMaskRenderer::CreateDescriptorSet()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount					= 0;
		descriptorCountDesc.TextureDescriptorCount					= 0;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 1;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 2;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 0;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 0;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "Paint Mask Renderer Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= 512;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		return true;
	}

	bool PaintMaskRenderer::CreateShaders()
	{
		m_VertexShaderGUID	= ResourceManager::LoadShaderFromFile("/MeshPainting/Unwrap.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderGUID	= ResourceManager::LoadShaderFromFile("/MeshPainting/Unwrap.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderGUID != GUID_NONE && m_PixelShaderGUID != GUID_NONE;
	}

	bool PaintMaskRenderer::CreateCommandLists()
	{
		m_ppRenderCommandAllocators = DBG_NEW CommandAllocator * [m_BackBufferCount];
		m_ppRenderCommandLists = DBG_NEW CommandList * [m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("Paint Mask Renderer Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName			= "Paint Mask Renderer Render Command List " + std::to_string(b);
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

	bool PaintMaskRenderer::CreateRenderPass(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		UNREFERENCED_VARIABLE(pPreInitDesc);

		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_R8G8_UINT;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		colorAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = { ETextureState::TEXTURE_STATE_RENDER_TARGET };

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass		= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass		= 0;
		subpassDependencyDesc.SrcAccessMask		= 0;
		subpassDependencyDesc.DstAccessMask		= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask		= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask		= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "Paint Mask Renderer Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool PaintMaskRenderer::CreatePipelineState()
	{
		m_PipelineStateBothID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID, COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G);
		m_PipelineStateServerID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID, COLOR_COMPONENT_FLAG_R);
		m_PipelineStateClientID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID, COLOR_COMPONENT_FLAG_G);

		return true;
	}

	uint64 PaintMaskRenderer::InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader, FColorComponentFlags colorComponentFlags)
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName							= "Paint Mask Renderer Pipeline State";
		pipelineStateDesc.RenderPass						= m_RenderPass;
		pipelineStateDesc.PipelineLayout					= m_PipelineLayout;
		pipelineStateDesc.InputAssembly.PrimitiveTopology	= EPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineStateDesc.RasterizerState.LineWidth			= 1.f;
		pipelineStateDesc.RasterizerState.PolygonMode		= EPolygonMode::POLYGON_MODE_FILL;
		pipelineStateDesc.RasterizerState.CullMode			= ECullMode::CULL_MODE_NONE;

		pipelineStateDesc.DepthStencilState = {};
		pipelineStateDesc.DepthStencilState.DepthTestEnable		= false;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable	= false;

		pipelineStateDesc.BlendState.BlendAttachmentStates =
		{
			{
				EBlendOp::BLEND_OP_NONE,
				EBlendFactor::BLEND_FACTOR_NONE,
				EBlendFactor::BLEND_FACTOR_NONE,
				EBlendOp::BLEND_OP_NONE,
				EBlendFactor::BLEND_FACTOR_NONE,
				EBlendFactor::BLEND_FACTOR_NONE,
				colorComponentFlags,
				false
			}
		};

		pipelineStateDesc.VertexShader.ShaderGUID	= vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID	= pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}
}