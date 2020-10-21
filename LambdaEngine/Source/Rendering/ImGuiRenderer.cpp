#include "Engine/EngineLoop.h"

#include "Rendering/ImGuiRenderer.h"
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

#include "Application/API/Events/KeyEvents.h"
#include "Application/API/Events/MouseEvents.h"

#include "Debug/Profiler.h"

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#pragma warning( push, 0 )
#include <imgui.h>
#pragma warning( pop )
#include <imnodes.h>

#include <d3d12.h>

namespace LambdaEngine
{
	ImGuiRenderer* ImGuiRenderer::s_pRendererInstance = nullptr;

	/*
	* ImGuiRenderer
	*/
	ImGuiRenderer::ImGuiRenderer(const GraphicsDevice* pGraphicsDevice, ImGuiRendererDesc* pDesc)
		: m_pGraphicsDevice(pGraphicsDevice)
	{
		VALIDATE(s_pRendererInstance == nullptr);
		VALIDATE(pDesc);

		s_pRendererInstance = this;
		m_pDesc = pDesc;
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		VALIDATE(s_pRendererInstance != nullptr);
		s_pRendererInstance = nullptr;

		if (m_ppRenderCommandLists != nullptr && m_ppRenderCommandAllocators != nullptr)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppRenderCommandLists[b]);
				SAFERELEASE(m_ppRenderCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppRenderCommandLists);
			SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
		}

		EventHandler eventHandler(this, &ImGuiRenderer::OnEvent);
		EventQueue::UnregisterEventHandler<MouseMovedEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<MouseScrolledEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<MouseButtonReleasedEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<MouseButtonClickedEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<KeyPressedEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<KeyReleasedEvent>(eventHandler);
		EventQueue::UnregisterEventHandler<KeyTypedEvent>(eventHandler);

		imnodes::Shutdown();
		ImGui::DestroyContext();
	}

	bool ImGuiRenderer::Init()
	{
		uint32 backBufferCount = m_pDesc->BackBufferCount;
		m_BackBuffers.Resize(backBufferCount);

		if (!InitImGui())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to initialize ImGui");
			return false;
		}

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create copy Command List");
			return false;
		}

		if (!CreateBuffers(m_pDesc->VertexBufferSize, m_pDesc->IndexBufferSize))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create buffers");
			return false;
		}

		if (!CreateTextures())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create textures");
			return false;
		}

		if (!CreateSamplers())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create samplers");
			return false;
		}

		if (!CreatePipelineLayout())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create PipelineLayout");
			return false;
		}

		if (!CreateDescriptorSet())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create DescriptorSet");
			return false;
		}

		if (!CreateShaders())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create Shaders");
			return false;
		}

		m_DescriptorSet->WriteTextureDescriptors(&m_FontTextureView, &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);

		EventHandler eventHandler(this, &ImGuiRenderer::OnEvent);

		bool result = true;
		result = result && EventQueue::RegisterEventHandler<MouseMovedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseScrolledEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseButtonReleasedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<MouseButtonClickedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<KeyPressedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<KeyReleasedEvent>(eventHandler);
		result = result && EventQueue::RegisterEventHandler<KeyTypedEvent>(eventHandler);
		return result;
	}

	void ImGuiRenderer::DrawUI(ImGuiDrawFunc drawFunc)
	{
		std::scoped_lock<SpinLock> lock(m_DrawCallsLock);
		m_DeferredDrawCalls.EmplaceBack(drawFunc);
	}

	bool ImGuiRenderer::RenderGraphInit(const CustomRendererRenderGraphInitDesc* pPreInitDesc)
	{
		VALIDATE(pPreInitDesc);
		VALIDATE(pPreInitDesc->ColorAttachmentCount == 1);

		m_BackBufferCount = pPreInitDesc->BackBufferCount;

		if (!CreateRenderCommandLists())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create render command lists");
			return false;
		}

		if (!CreateRenderPass(&pPreInitDesc->pColorAttachmentDesc[0]))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create RenderPass");
			return false;
		}

		if (!CreatePipelineState())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create PipelineState");
			return false;
		}

		return true;
	}

	void ImGuiRenderer::PreBuffersDescriptorSetWrite()
	{
	}

	void ImGuiRenderer::PreTexturesDescriptorSetWrite()
	{
	}

	//void ImGuiRenderer::UpdateParameters(void* pData)
	//{
	//	UNREFERENCED_VARIABLE(pData);
	//}

	/*void ImGuiRenderer::UpdatePushConstants(void* pData, uint32 dataSize)
	{
		UNREFERENCED_VARIABLE(pData);
		UNREFERENCED_VARIABLE(dataSize);
	}*/

	void ImGuiRenderer::Update(Timestamp delta, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		UNREFERENCED_VARIABLE(delta);
		UNREFERENCED_VARIABLE(modFrameIndex);
		UNREFERENCED_VARIABLE(backBufferIndex);
	}

	void ImGuiRenderer::UpdateTextureResource(
		const String& resourceName, 
		const TextureView* const* ppPerImageTextureViews, 
		const TextureView* const* ppPerSubImageTextureViews, 
		uint32 imageCount, 
		uint32 subImageCount,
		bool backBufferBound)
	{
		if (subImageCount == 1 || backBufferBound || subImageCount == imageCount)
		{
			if (resourceName == RENDER_GRAPH_BACK_BUFFER_ATTACHMENT)
			{
				for (uint32 i = 0; i < imageCount; i++)
				{
					m_BackBuffers[i] = MakeSharedRef(ppPerSubImageTextureViews[i]);
				}
			}
			else if (imageCount > 1 && imageCount == subImageCount)
			{
				for (uint32 imageIndex = 0; imageIndex < imageCount; imageIndex++)
				{
					String currentResourceName = resourceName + "_" + std::to_string(imageIndex);

					if (!m_TextureResourceNameDescriptorSetsMap.contains(currentResourceName))
					{
						TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[currentResourceName];
						descriptorSets.Resize(1);
						TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Custom Texture Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
						descriptorSets[0] = descriptorSet;
						descriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[imageIndex], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
					}
					else
					{
						TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[currentResourceName];
						descriptorSets[0]->WriteTextureDescriptors(&ppPerImageTextureViews[imageIndex], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
					}
				}
			}
			else
			{
				auto textureIt = m_TextureResourceNameDescriptorSetsMap.find(resourceName);
				if (textureIt == m_TextureResourceNameDescriptorSetsMap.end())
				{
					TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[resourceName];
					if (backBufferBound)
					{
						uint32 backBufferCount = m_BackBuffers.GetSize();
						descriptorSets.Resize(backBufferCount);
						for (uint32 b = 0; b < backBufferCount; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Custom Texture Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
							descriptorSets[b] = descriptorSet;

							descriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[b], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
						}
					}
					else
					{
						descriptorSets.Resize(imageCount);
						for (uint32 b = 0; b < imageCount; b++)
						{
							TSharedRef<DescriptorSet> descriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Custom Texture Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());
							descriptorSets[b] = descriptorSet;

							descriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[b], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
						}
					}
				}
				else
				{
					TArray<TSharedRef<DescriptorSet>>& descriptorSets = m_TextureResourceNameDescriptorSetsMap[resourceName];
					if (backBufferBound)
					{
						uint32 backBufferCount = m_BackBuffers.GetSize();
						if (descriptorSets.GetSize() == backBufferCount)
						{
							for (uint32 b = 0; b < backBufferCount; b++)
							{
								TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
								descriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[b], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
							}
						}
					}
					else
					{
						if (descriptorSets.GetSize() == imageCount)
						{
							for (uint32 b = 0; b < imageCount; b++)
							{
								TSharedRef<DescriptorSet> descriptorSet = descriptorSets[b];
								descriptorSet->WriteTextureDescriptors(&ppPerImageTextureViews[b], &m_Sampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER, true);
							}
						}
						else
						{
							LOG_ERROR("[ImGuiRenderer]: Texture count changed between calls to UpdateTextureResource for resource \"%s\"", resourceName.c_str());
						}
					}
				}
			}
		}
		else
		{
			LOG_WARNING("[ImGuiRenderer]: Textures with subImageCount or imageCount != subImageCount and not BackBufferBound is not implemented. imageCount: %d, subImageCount: %d", imageCount, subImageCount);
		}
	}

	void ImGuiRenderer::UpdateBufferResource(const String& resourceName, const Buffer* const* ppBuffers, uint64* pOffsets, uint64* pSizesInBytes, uint32 count, bool backBufferBound)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(ppBuffers);
		UNREFERENCED_VARIABLE(pOffsets);
		UNREFERENCED_VARIABLE(pSizesInBytes);
		UNREFERENCED_VARIABLE(count);
		UNREFERENCED_VARIABLE(backBufferBound);
	}

	void ImGuiRenderer::UpdateAccelerationStructureResource(const String& resourceName, const AccelerationStructure* pAccelerationStructure)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pAccelerationStructure);
	}

	void ImGuiRenderer::UpdateDrawArgsResource(const String& resourceName, const DrawArg* pDrawArgs, uint32 count)
	{
		UNREFERENCED_VARIABLE(resourceName);
		UNREFERENCED_VARIABLE(pDrawArgs);
		UNREFERENCED_VARIABLE(count);

		for (uint32 i = 0; i < count; i++)
		{
			const DrawArg& drawArg = pDrawArgs[i];
			
			if (drawArg.HasExtensions)
			{
				for (uint32 groupIndex = 0; groupIndex < drawArg.InstanceCount; groupIndex++)
				{
					DrawArgExtensionGroup* extensionGroup = drawArg.ppExtensionGroups[groupIndex];
					if (extensionGroup)
					{
						for (uint32 extensionIndex = 0; extensionIndex < extensionGroup->ExtensionCount; extensionIndex++)
						{
							DrawArgExtensionData& extensionData = extensionGroup->pExtensions[extensionIndex];
							for (uint32 textureIndex = 0; textureIndex < extensionData.TextureCount; textureIndex++)
							{
								UpdateTextureResource(extensionData.ppTextures[textureIndex]->GetDesc().DebugName,
									&extensionData.ppTextureViews[textureIndex], &extensionData.ppTextureViews[textureIndex], 1, 1, false);
							}
						}
					}
				}
			}
		}
	}

	void ImGuiRenderer::Render(
		uint32 modFrameIndex,
		uint32 backBufferIndex,
		CommandList** ppFirstExecutionStage,
		CommandList** ppSecondaryExecutionStage,
		bool sleeping)
	{
		UNREFERENCED_VARIABLE(ppSecondaryExecutionStage);

		if (sleeping)
			return;

		// Update imgui for this frame
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		uint32 windowWidth	= window->GetWidth();
		uint32 windowHeight = window->GetHeight();

		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = static_cast<float32>(EngineLoop::GetDeltaTime().AsSeconds());
		io.DisplaySize = ImVec2(static_cast<float32>(windowWidth), static_cast<float32>(windowHeight));
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		// Render all ImGui calls
		ImGui::NewFrame();

		{
			std::scoped_lock<SpinLock> lock(m_DrawCallsLock);
			for (ImGuiDrawFunc& func : m_DeferredDrawCalls)
			{
				func();
			}
			m_DeferredDrawCalls.Clear();
		}

		ImGui::EndFrame();
		ImGui::Render();


		//Start drawing
		ImDrawData* pDrawData = ImGui::GetDrawData();
		TSharedRef<const TextureView> backBuffer = m_BackBuffers[backBufferIndex];
		uint32 width	= backBuffer->GetDesc().pTexture->GetDesc().Width;
		uint32 height	= backBuffer->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass			= m_RenderPass.Get();
		beginRenderPassDesc.ppRenderTargets		= &backBuffer;
		beginRenderPassDesc.RenderTargetCount	= 1;
		beginRenderPassDesc.pDepthStencil		= nullptr;
		beginRenderPassDesc.Width				= width;
		beginRenderPassDesc.Height				= height;
		beginRenderPassDesc.Flags				= FRenderPassBeginFlag::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors		= nullptr;
		beginRenderPassDesc.ClearColorCount		= 0;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		CommandList* pCommandList = m_ppRenderCommandLists[modFrameIndex];

		// Render to screen
		if (pDrawData == nullptr || pDrawData->CmdListsCount == 0)
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

		Profiler::GetGPUProfiler()->GetTimestamp(pCommandList);

		m_ppRenderCommandAllocators[modFrameIndex]->Reset();
		pCommandList->Begin(nullptr);

		Profiler::GetGPUProfiler()->ResetTimestamp(pCommandList);
		Profiler::GetGPUProfiler()->StartTimestamp(pCommandList);

		{
			TSharedRef<Buffer> vertexCopyBuffer	= m_VertexCopyBuffers[modFrameIndex];
			TSharedRef<Buffer> indexCopyBuffer	= m_IndexCopyBuffers[modFrameIndex];

			uint32 vertexBufferSize		= 0;
			uint32 indexBufferSize		= 0;
			byte* pVertexMapping		= reinterpret_cast<byte*>(vertexCopyBuffer->Map());
			byte* pIndexMapping			= reinterpret_cast<byte*>(indexCopyBuffer->Map());

			for (int n = 0; n < pDrawData->CmdListsCount; n++)
			{
				const ImDrawList* pDrawList = pDrawData->CmdLists[n];

				memcpy(pVertexMapping + vertexBufferSize,	pDrawList->VtxBuffer.Data, pDrawList->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(pIndexMapping + indexBufferSize,		pDrawList->IdxBuffer.Data, pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx));

				vertexBufferSize	+= pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
				indexBufferSize		+= pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
			}

			vertexCopyBuffer->Unmap();
			indexCopyBuffer->Unmap();
			pCommandList->CopyBuffer(vertexCopyBuffer.Get(), 0, m_VertexBuffer.Get(), 0, vertexBufferSize);
			pCommandList->CopyBuffer(indexCopyBuffer.Get(), 0, m_IndexBuffer.Get(), 0, indexBufferSize);
		}

		pCommandList->BeginRenderPass(&beginRenderPassDesc);
	
		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float32)width;
		viewport.Height		= (float32)height;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pCommandList->SetViewports(&viewport, 0, 1);

		uint64 offset = 0;
		pCommandList->BindVertexBuffers(&m_VertexBuffer, 0, &offset, 1);
		pCommandList->BindIndexBuffer(m_IndexBuffer.Get(), 0, EIndexType::INDEX_TYPE_UINT16);

		// Setup scale and translation:
		// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
		{
			float32 pScale[2];
			pScale[0] = 2.0f / pDrawData->DisplaySize.x;
			pScale[1] = 2.0f / pDrawData->DisplaySize.y;
			float pTranslate[2];
			pTranslate[0] = -1.0f - pDrawData->DisplayPos.x * pScale[0];
			pTranslate[1] = -1.0f - pDrawData->DisplayPos.y * pScale[1];

			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, pScale,		2 * sizeof(float32), 0 * sizeof(float32));
			pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, pTranslate,	2 * sizeof(float32), 2 * sizeof(float32));
		}

		// Will project scissor/clipping rectangles into framebuffer space
		ImVec2 clipOffset		= pDrawData->DisplayPos;       // (0,0) unless using multi-viewports
		ImVec2 clipScale		= pDrawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int32 globalVertexOffset = 0;
		int32 globalIndexOffset = 0;

		for (int32 n = 0; n < pDrawData->CmdListsCount; n++)
		{
			const ImDrawList* pCmdList = pDrawData->CmdLists[n];
			for (int32 i = 0; i < pCmdList->CmdBuffer.Size; i++)
			{
				const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[i];
				// Project scissor/clipping rectangles into framebuffer space
				ImVec4 clipRect;
				clipRect.x = (pCmd->ClipRect.x - clipOffset.x) * clipScale.x;
				clipRect.y = (pCmd->ClipRect.y - clipOffset.y) * clipScale.y;
				clipRect.z = (pCmd->ClipRect.z - clipOffset.x) * clipScale.x;
				clipRect.w = (pCmd->ClipRect.w - clipOffset.y) * clipScale.y;

				if (clipRect.x < viewport.Width && clipRect.y < viewport.Height && clipRect.z >= 0.0f && clipRect.w >= 0.0f)
				{
					// Negative offsets are illegal for vkCmdSetScissor
					if (clipRect.x < 0.0f)
						clipRect.x = 0.0f;
					if (clipRect.y < 0.0f)
						clipRect.y = 0.0f;

					// Apply scissor/clipping rectangle
					ScissorRect scissorRect = {};
					scissorRect.x				= (int32)clipRect.x;
					scissorRect.y				= (int32)clipRect.y;
					scissorRect.Width			= uint32(clipRect.z - clipRect.x);
					scissorRect.Height			= uint32(clipRect.w - clipRect.y);

					pCommandList->SetScissorRects(&scissorRect, 0, 1);

					if (pCmd->TextureId)
					{
						ImGuiTexture*		pImGuiTexture	= reinterpret_cast<ImGuiTexture*>(pCmd->TextureId);
						auto textureIt = m_TextureResourceNameDescriptorSetsMap.find(pImGuiTexture->ResourceName);

						if (textureIt == m_TextureResourceNameDescriptorSetsMap.end()) continue;

						GUID_Lambda vertexShaderGUID	= pImGuiTexture->VertexShaderGUID == GUID_NONE	? m_VertexShaderGUID	: pImGuiTexture->VertexShaderGUID;
						GUID_Lambda pixelShaderGUID		= pImGuiTexture->PixelShaderGUID == GUID_NONE	? m_PixelShaderGUID		: pImGuiTexture->PixelShaderGUID;

						auto vertexShaderIt = m_ShadersIDToPipelineStateIDMap.find(vertexShaderGUID);

						if (vertexShaderIt != m_ShadersIDToPipelineStateIDMap.end())
						{
							auto pixelShaderIt = vertexShaderIt->second.find(pixelShaderGUID);

							if (pixelShaderIt != vertexShaderIt->second.end())
							{
								PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pixelShaderIt->second);
								pCommandList->BindGraphicsPipeline(pPipelineState);
							}
							else
							{
								uint64 pipelineGUID = InternalCreatePipelineState(vertexShaderGUID, pixelShaderGUID);
								VALIDATE(pipelineGUID != 0);

								vertexShaderIt->second.insert({ pixelShaderGUID, pipelineGUID });

								PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pipelineGUID);
								pCommandList->BindGraphicsPipeline(pPipelineState);
							}
						}
						else
						{
							uint64 pipelineGUID = InternalCreatePipelineState(vertexShaderGUID, pixelShaderGUID);
							VALIDATE(pipelineGUID != 0);

							THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
							pixelShaderToPipelineStateMap.insert({ pixelShaderGUID, pipelineGUID });
							m_ShadersIDToPipelineStateIDMap.insert({ vertexShaderGUID, pixelShaderToPipelineStateMap });

							PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(pipelineGUID);
							pCommandList->BindGraphicsPipeline(pPipelineState);
						}

						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, pImGuiTexture->ChannelMul,			4 * sizeof(float32),	4 * sizeof(float32));
						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, pImGuiTexture->ChannelAdd,			4 * sizeof(float32),	8 * sizeof(float32));
						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, &pImGuiTexture->ReservedIncludeMask,	sizeof(uint32),		12 * sizeof(float32));

						const TArray<TSharedRef<DescriptorSet>>& descriptorSets = textureIt->second;
						//Todo: Allow other sizes than 1
						if (descriptorSets.GetSize() == 1)
						{
							pCommandList->BindDescriptorSetGraphics(descriptorSets[0].Get(), m_PipelineLayout.Get(), 0);
						}
						else
						{
							pCommandList->BindDescriptorSetGraphics(descriptorSets[backBufferIndex].Get(), m_PipelineLayout.Get(), 0);
						}
					}
					else
					{
						constexpr const float32 DEFAULT_CHANNEL_MUL[4]					= { 1.0f, 1.0f, 1.0f, 1.0f };
						constexpr const float32 DEFAULT_CHANNEL_ADD[4]					= { 0.0f, 0.0f, 0.0f, 0.0f };
						constexpr const uint32 DEFAULT_CHANNEL_RESERVED_INCLUDE_MASK	= 0x00008421;  //0000 0000 0000 0000 1000 0100 0010 0001

						PipelineState* pPipelineState = PipelineStateManager::GetPipelineState(m_PipelineStateID);
						pCommandList->BindGraphicsPipeline(pPipelineState);

						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, DEFAULT_CHANNEL_MUL,						4 * sizeof(float32),	4 * sizeof(float32));
						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, DEFAULT_CHANNEL_ADD,						4 * sizeof(float32),	8 * sizeof(float32));
						pCommandList->SetConstantRange(m_PipelineLayout.Get(), FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, &DEFAULT_CHANNEL_RESERVED_INCLUDE_MASK,		sizeof(uint32),		12 * sizeof(float32));

						pCommandList->BindDescriptorSetGraphics(m_DescriptorSet.Get(), m_PipelineLayout.Get(), 0);
					}

					// Draw
					pCommandList->DrawIndexInstanced(pCmd->ElemCount, 1, pCmd->IdxOffset + globalIndexOffset, pCmd->VtxOffset + globalVertexOffset, 0);
				}
			}

			globalIndexOffset	+= pCmdList->IdxBuffer.Size;
			globalVertexOffset	+= pCmdList->VtxBuffer.Size;
		}

		Profiler::GetGPUProfiler()->EndTimestamp(pCommandList);

		pCommandList->EndRenderPass();
		pCommandList->End();

		(*ppFirstExecutionStage) = pCommandList;
	}

	bool ImGuiRenderer::OnEvent(const Event& event)
	{
		ImGuiIO& io = ImGui::GetIO();
		if (IsEventOfType<MouseMovedEvent>(event))
		{
			MouseMovedEvent mouseEvent = EventCast<MouseMovedEvent>(event);
			io.MousePos = ImVec2(float32(mouseEvent.Position.x), float32(mouseEvent.Position.y));

			return true;
		}
		else if (IsEventOfType<MouseButtonClickedEvent>(event))
		{
			MouseButtonClickedEvent mouseEvent = EventCast<MouseButtonClickedEvent>(event);
			io.MouseDown[mouseEvent.Button - 1] = true;

			return true;
		}
		else if (IsEventOfType<MouseButtonReleasedEvent>(event))
		{
			MouseButtonReleasedEvent mouseEvent = EventCast<MouseButtonReleasedEvent>(event);
			io.MouseDown[mouseEvent.Button - 1] = false;

			return true;
		}
		else if (IsEventOfType<MouseScrolledEvent>(event))
		{
			MouseScrolledEvent mouseEvent = EventCast<MouseScrolledEvent>(event);
			io.MouseWheelH	+= static_cast<float32>(mouseEvent.DeltaX);
			io.MouseWheel	+= static_cast<float32>(mouseEvent.DeltaY);

			return true;
		}
		else if (IsEventOfType<KeyPressedEvent>(event))
		{
			KeyPressedEvent keyEvent = EventCast<KeyPressedEvent>(event);
			io.KeysDown[keyEvent.Key] = true;
			io.KeyCtrl	= keyEvent.ModiferState.IsCtrlDown();
			io.KeyShift = keyEvent.ModiferState.IsShiftDown();
			io.KeyAlt	= keyEvent.ModiferState.IsAltDown();
			io.KeySuper	= keyEvent.ModiferState.IsSuperDown();

			return true;
		}
		else if (IsEventOfType<KeyReleasedEvent>(event))
		{
			KeyReleasedEvent keyEvent = EventCast<KeyReleasedEvent>(event);
			io.KeysDown[keyEvent.Key] = false;
			io.KeyCtrl	= keyEvent.ModiferState.IsCtrlDown();
			io.KeyShift = keyEvent.ModiferState.IsShiftDown();
			io.KeyAlt	= keyEvent.ModiferState.IsAltDown();
			io.KeySuper	= keyEvent.ModiferState.IsSuperDown();

			return true;
		}
		else if (IsEventOfType<KeyTypedEvent>(event))
		{
			KeyTypedEvent keyEvent = EventCast<KeyTypedEvent>(event);
			io.AddInputCharacter(keyEvent.Character);

			return true;
		}

		return false;
	}

	ImGuiContext* ImGuiRenderer::GetImguiContext()
	{
		return ImGui::GetCurrentContext();
	}

	ImGuiRenderer& ImGuiRenderer::Get()
	{
		VALIDATE(s_pRendererInstance != nullptr);
		return *s_pRendererInstance;
	}

	bool ImGuiRenderer::InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		imnodes::Initialize();

		ImGuiIO& io = ImGui::GetIO();
		io.BackendPlatformName = "Lambda Engine";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

		io.KeyMap[ImGuiKey_Tab]			= EKey::KEY_TAB;
		io.KeyMap[ImGuiKey_LeftArrow]	= EKey::KEY_LEFT;
		io.KeyMap[ImGuiKey_RightArrow]	= EKey::KEY_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow]		= EKey::KEY_UP;
		io.KeyMap[ImGuiKey_DownArrow]	= EKey::KEY_DOWN;
		io.KeyMap[ImGuiKey_PageUp]		= EKey::KEY_PAGE_UP;
		io.KeyMap[ImGuiKey_PageDown]	= EKey::KEY_PAGE_DOWN;
		io.KeyMap[ImGuiKey_Home]		= EKey::KEY_HOME;
		io.KeyMap[ImGuiKey_End]			= EKey::KEY_END;
		io.KeyMap[ImGuiKey_Insert]		= EKey::KEY_INSERT;
		io.KeyMap[ImGuiKey_Delete]		= EKey::KEY_DELETE;
		io.KeyMap[ImGuiKey_Backspace]	= EKey::KEY_BACKSPACE;
		io.KeyMap[ImGuiKey_Space]		= EKey::KEY_SPACE;
		io.KeyMap[ImGuiKey_Enter]		= EKey::KEY_ENTER;
		io.KeyMap[ImGuiKey_Escape]		= EKey::KEY_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = EKey::KEY_KEYPAD_ENTER;
		io.KeyMap[ImGuiKey_A]			= EKey::KEY_A;
		io.KeyMap[ImGuiKey_C]			= EKey::KEY_C;
		io.KeyMap[ImGuiKey_V]			= EKey::KEY_V;
		io.KeyMap[ImGuiKey_X]			= EKey::KEY_X;
		io.KeyMap[ImGuiKey_Y]			= EKey::KEY_Y;
		io.KeyMap[ImGuiKey_Z]			= EKey::KEY_Z;

#ifdef LAMBDA_PLATFORM_WINDOWS
		TSharedRef<Window> window = CommonApplication::Get()->GetMainWindow();
		io.ImeWindowHandle = window->GetHandle();
#endif

		//Todo: Implement clipboard handling
		//io.SetClipboardTextFn = ImGuiSetClipboardText;
		//io.GetClipboardTextFn = ImGuiGetClipboardText;
		//io.ClipboardUserData = pWindow;

		ImGui::StyleColorsDark();
		ImGui::GetStyle().WindowRounding	= 0.0f;
		ImGui::GetStyle().ChildRounding		= 0.0f;
		ImGui::GetStyle().FrameRounding		= 0.0f;
		ImGui::GetStyle().GrabRounding		= 0.0f;
		ImGui::GetStyle().PopupRounding		= 0.0f;
		ImGui::GetStyle().ScrollbarRounding = 0.0f;

		// Disable anti-aliasing for the rendering to work better. (Unknown reason why it does not work when it's on)
		ImGuiStyle& style = ImGui::GetStyle();
		style.AntiAliasedLines = false;
		style.AntiAliasedLinesUseTex = true;
		style.AntiAliasedFill = true;
		style.FrameBorderSize = 0.f;

		return true;
	}

	bool ImGuiRenderer::CreateCopyCommandList()
	{
		m_CopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("ImGui Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

		if (!m_CopyCommandAllocator)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.DebugName		= "ImGui Copy Command List";
		commandListDesc.CommandListType	= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
		commandListDesc.Flags			= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_CopyCommandList = m_pGraphicsDevice->CreateCommandList(m_CopyCommandAllocator.Get(), &commandListDesc);

		if (!m_CopyCommandList)
		{
			return false;
		}

		return true;
	}

	bool ImGuiRenderer::CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize)
	{
		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.DebugName		= "ImGui Vertex Buffer";
		vertexBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_GPU;
		vertexBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_VERTEX_BUFFER;
		vertexBufferDesc.SizeInBytes	= vertexBufferSize;

		m_VertexBuffer = m_pGraphicsDevice->CreateBuffer(&vertexBufferDesc);
		if (!m_VertexBuffer)
		{
			return false;
		}

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.DebugName	= "ImGui Index Buffer";
		indexBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		indexBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_DST | FBufferFlag::BUFFER_FLAG_INDEX_BUFFER;
		indexBufferDesc.SizeInBytes	= vertexBufferSize;

		m_IndexBuffer = m_pGraphicsDevice->CreateBuffer(&indexBufferDesc);
		if (!m_IndexBuffer)
		{
			return false;
		}

		BufferDesc vertexCopyBufferDesc = {};
		vertexCopyBufferDesc.DebugName		= "ImGui Vertex Copy Buffer";
		vertexCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		vertexCopyBufferDesc.Flags			= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		vertexCopyBufferDesc.SizeInBytes	= vertexBufferSize;

		BufferDesc indexCopyBufferDesc = {};
		indexCopyBufferDesc.DebugName	= "ImGui Index Copy Buffer";
		indexCopyBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		indexCopyBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		indexCopyBufferDesc.SizeInBytes	= indexBufferSize;

		uint32 backBufferCount = m_BackBuffers.GetSize();
		m_VertexCopyBuffers.Resize(backBufferCount);
		m_IndexCopyBuffers.Resize(backBufferCount);
		for (uint32 b = 0; b < backBufferCount; b++)
		{
			TSharedRef<Buffer> vertexBuffer = m_pGraphicsDevice->CreateBuffer(&vertexCopyBufferDesc);
			TSharedRef<Buffer> indexBuffer = m_pGraphicsDevice->CreateBuffer(&indexCopyBufferDesc);
			if (vertexBuffer != nullptr && indexBuffer != nullptr)
			{
				m_VertexCopyBuffers[b] = vertexBuffer;
				m_IndexCopyBuffers[b] = indexBuffer;
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool ImGuiRenderer::CreateTextures()
	{
		ImGuiIO& io = ImGui::GetIO();

		uint8* pPixels = nullptr;
		int32 width = 0;
		int32 height = 0;
		io.Fonts->GetTexDataAsRGBA32(&pPixels, &width, &height);

		int64 textureSize = 4 * width * height;

		BufferDesc fontBufferDesc = {};
		fontBufferDesc.DebugName	= "ImGui Font Buffer";
		fontBufferDesc.MemoryType	= EMemoryType::MEMORY_TYPE_CPU_VISIBLE;
		fontBufferDesc.Flags		= FBufferFlag::BUFFER_FLAG_COPY_SRC;
		fontBufferDesc.SizeInBytes	= textureSize;

		TSharedRef<Buffer> fontBuffer = m_pGraphicsDevice->CreateBuffer(&fontBufferDesc);
		if (fontBuffer == nullptr)
		{
			return false;
		}

		void* pMapped = fontBuffer->Map();
		memcpy(pMapped, pPixels, textureSize);
		fontBuffer->Unmap();

		TextureDesc fontTextureDesc = {};
		fontTextureDesc.DebugName	= "ImGui Font Texture";
		fontTextureDesc.MemoryType  = EMemoryType::MEMORY_TYPE_GPU;
		fontTextureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		fontTextureDesc.Flags		= FTextureFlag::TEXTURE_FLAG_COPY_DST | FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE;
		fontTextureDesc.Width		= width;
		fontTextureDesc.Height		= height;
		fontTextureDesc.Depth		= 1;
		fontTextureDesc.ArrayCount	= 1;
		fontTextureDesc.Miplevels	= 1;
		fontTextureDesc.SampleCount = 1;

		m_FontTexture = m_pGraphicsDevice->CreateTexture(&fontTextureDesc);
		if (!m_FontTexture)
		{
			return false;
		}

		CopyTextureFromBufferDesc copyDesc = {};
		copyDesc.SrcOffset		= 0;
		copyDesc.SrcRowPitch	= 0;
		copyDesc.SrcHeight		= 0;
		copyDesc.Width			= width;
		copyDesc.Height			= height;
		copyDesc.Depth			= 1;
		copyDesc.Miplevel		= 0;
		copyDesc.MiplevelCount  = 1;
		copyDesc.ArrayIndex		= 0;
		copyDesc.ArrayCount		= 1;

		m_CopyCommandAllocator->Reset();
		m_CopyCommandList->Begin(nullptr);

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture				= m_FontTexture.Get();
		transitionToCopyDstBarrier.StateBefore			= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags	= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags			= fontTextureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel				= 0;
		transitionToCopyDstBarrier.MiplevelCount		= fontTextureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex			= 0;
		transitionToCopyDstBarrier.ArrayCount			= fontTextureDesc.ArrayCount;

		m_CopyCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);
		m_CopyCommandList->CopyTextureFromBuffer(fontBuffer.Get(), m_FontTexture.Get(), copyDesc);

		PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
		transitionToShaderReadBarrier.pTexture				= m_FontTexture.Get();
		transitionToShaderReadBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToShaderReadBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		transitionToShaderReadBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToShaderReadBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_NONE;
		transitionToShaderReadBarrier.SrcMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToShaderReadBarrier.DstMemoryAccessFlags	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
		transitionToShaderReadBarrier.TextureFlags			= fontTextureDesc.Flags;
		transitionToShaderReadBarrier.Miplevel				= 0;
		transitionToShaderReadBarrier.MiplevelCount			= fontTextureDesc.Miplevels;
		transitionToShaderReadBarrier.ArrayIndex			= 0;
		transitionToShaderReadBarrier.ArrayCount			= fontTextureDesc.ArrayCount;

		m_CopyCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);
		m_CopyCommandList->End();

		RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&m_CopyCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, nullptr, 0);
		RenderAPI::GetGraphicsQueue()->Flush();

		TextureViewDesc fontTextureViewDesc = {};
		fontTextureViewDesc.DebugName		= "ImGui Font Texture View";
		fontTextureViewDesc.pTexture		= m_FontTexture.Get();
		fontTextureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		fontTextureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		fontTextureViewDesc.MiplevelCount	= 1;
		fontTextureViewDesc.ArrayCount		= 1;
		fontTextureViewDesc.Miplevel		= 0;
		fontTextureViewDesc.ArrayIndex		= 0;

		m_FontTextureView = m_pGraphicsDevice->CreateTextureView(&fontTextureViewDesc);
		if (!m_FontTextureView)
		{
			return false;
		}

		return true;
	}

	bool ImGuiRenderer::CreateSamplers()
	{
		SamplerDesc samplerDesc = {};
		samplerDesc.DebugName			= "ImGui Sampler";
		samplerDesc.MinFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerDesc.MagFilter			= EFilterType::FILTER_TYPE_NEAREST;
		samplerDesc.MipmapMode			= EMipmapMode::MIPMAP_MODE_NEAREST;
		samplerDesc.AddressModeU		= ESamplerAddressMode::SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
		samplerDesc.AddressModeV		= samplerDesc.AddressModeU;
		samplerDesc.AddressModeW		= samplerDesc.AddressModeU;
		samplerDesc.MipLODBias			= 0.0f;
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		m_Sampler = m_pGraphicsDevice->CreateSampler(&samplerDesc);

		return m_Sampler != nullptr;
	}

	bool ImGuiRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc descriptorBindingDesc = {};
		descriptorBindingDesc.DescriptorType	= EDescriptorType::DESCRIPTOR_TYPE_SHADER_RESOURCE_COMBINED_SAMPLER;
		descriptorBindingDesc.DescriptorCount	= 1;
		descriptorBindingDesc.Binding			= 0;
		descriptorBindingDesc.ShaderStageMask	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc = {};
		descriptorSetLayoutDesc.DescriptorBindings		= { descriptorBindingDesc };

		ConstantRangeDesc constantRangeVertexDesc = { };
		constantRangeVertexDesc.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER;
		constantRangeVertexDesc.SizeInBytes			= 4 * sizeof(float32);
		constantRangeVertexDesc.OffsetInBytes		= 0;

		ConstantRangeDesc constantRangePixelDesc = { };
		constantRangePixelDesc.ShaderStageFlags	= FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER;
		constantRangePixelDesc.SizeInBytes		= 8 * sizeof(float32) + sizeof(uint32);
		constantRangePixelDesc.OffsetInBytes	= constantRangeVertexDesc.SizeInBytes;

		ConstantRangeDesc pConstantRanges[2] = { constantRangeVertexDesc, constantRangePixelDesc };

		PipelineLayoutDesc pipelineLayoutDesc = { };
		pipelineLayoutDesc.DebugName			= "ImGui Pipeline Layout";
		pipelineLayoutDesc.DescriptorSetLayouts	= { descriptorSetLayoutDesc };
		pipelineLayoutDesc.ConstantRanges		= { pConstantRanges[0], pConstantRanges[1] };

		m_PipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_PipelineLayout != nullptr;
	}

	bool ImGuiRenderer::CreateDescriptorSet()
	{
		DescriptorHeapInfo descriptorCountDesc = { };
		descriptorCountDesc.SamplerDescriptorCount					= 1;
		descriptorCountDesc.TextureDescriptorCount					= 1;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 64;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 1;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 1;

		DescriptorHeapDesc descriptorHeapDesc = { };
		descriptorHeapDesc.DebugName			= "ImGui Descriptor Heap";
		descriptorHeapDesc.DescriptorSetCount	= 256;
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_DescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);
		if (!m_DescriptorHeap)
		{
			return false;
		}

		m_DescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Descriptor Set", m_PipelineLayout.Get(), 0, m_DescriptorHeap.Get());

		return m_DescriptorSet != nullptr;
	}

	bool ImGuiRenderer::CreateShaders()
	{
		m_VertexShaderGUID		= ResourceManager::LoadShaderFromFile("/ImGui/ImGuiVertex.vert", FShaderStageFlag::SHADER_STAGE_FLAG_VERTEX_SHADER, EShaderLang::SHADER_LANG_GLSL);
		m_PixelShaderGUID		= ResourceManager::LoadShaderFromFile("/ImGui/ImGuiPixel.frag", FShaderStageFlag::SHADER_STAGE_FLAG_PIXEL_SHADER, EShaderLang::SHADER_LANG_GLSL);
		return m_VertexShaderGUID != GUID_NONE && m_PixelShaderGUID != GUID_NONE;
	}
	
	bool ImGuiRenderer::CreateRenderCommandLists()
	{
		if (m_ppRenderCommandLists != nullptr && m_ppRenderCommandAllocators != nullptr)
		{
			for (uint32 b = 0; b < m_BackBufferCount; b++)
			{
				SAFERELEASE(m_ppRenderCommandLists[b]);
				SAFERELEASE(m_ppRenderCommandAllocators[b]);
			}

			SAFEDELETE_ARRAY(m_ppRenderCommandLists);
			SAFEDELETE_ARRAY(m_ppRenderCommandAllocators);
		}

		m_ppRenderCommandAllocators	= DBG_NEW CommandAllocator*[m_BackBufferCount];
		m_ppRenderCommandLists		= DBG_NEW CommandList*[m_BackBufferCount];

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			m_ppRenderCommandAllocators[b] = m_pGraphicsDevice->CreateCommandAllocator("ImGui Render Command Allocator " + std::to_string(b), ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS);

			if (!m_ppRenderCommandAllocators[b])
			{
				return false;
			}

			CommandListDesc commandListDesc = {};
			commandListDesc.DebugName			= "ImGui Render Command List " + std::to_string(b);
			commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_TYPE_PRIMARY;
			commandListDesc.Flags				= FCommandListFlag::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

			m_ppRenderCommandLists[b] = m_pGraphicsDevice->CreateCommandList(m_ppRenderCommandAllocators[b], &commandListDesc);

			if (!m_ppRenderCommandLists[b])
			{
				return false;
			}

			CommandList* pCommandList = m_ppRenderCommandLists[b];

			Profiler::GetGPUProfiler()->AddTimestamp(pCommandList, "ImGui Render Command List");

			pCommandList->Begin(nullptr);
			Profiler::GetGPUProfiler()->ResetTimestamp(pCommandList);
			pCommandList->End();
			RenderAPI::GetGraphicsQueue()->ExecuteCommandLists(&pCommandList, 1, FPipelineStageFlag::PIPELINE_STAGE_FLAG_UNKNOWN, nullptr, 0, nullptr, 0);
			RenderAPI::GetGraphicsQueue()->Flush();
		}

		return true;
	}

	bool ImGuiRenderer::CreateRenderPass(RenderPassAttachmentDesc* pBackBufferAttachmentDesc)
	{
		if (m_RenderPass.Get())
			m_RenderPass.Reset();

		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= pBackBufferAttachmentDesc->InitialState;
		colorAttachmentDesc.FinalState		= pBackBufferAttachmentDesc->FinalState;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates			= { ETextureState::TEXTURE_STATE_RENDER_TARGET };
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DONT_CARE;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask	= 0;
		subpassDependencyDesc.DstAccessMask	= FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= "ImGui Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_RenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return true;
	}

	bool ImGuiRenderer::CreatePipelineState()
	{
		m_PipelineStateID = InternalCreatePipelineState(m_VertexShaderGUID, m_PixelShaderGUID);
		VALIDATE(m_PipelineStateID != 0);

		THashTable<GUID_Lambda, uint64> pixelShaderToPipelineStateMap;
		pixelShaderToPipelineStateMap.insert({ m_PixelShaderGUID, m_PipelineStateID });
		m_ShadersIDToPipelineStateIDMap.insert({ m_VertexShaderGUID, pixelShaderToPipelineStateMap });

		return true;
	}

	uint64 ImGuiRenderer::InternalCreatePipelineState(GUID_Lambda vertexShader, GUID_Lambda pixelShader)
	{
		ManagedGraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.DebugName			= "ImGui Pipeline State";
		pipelineStateDesc.RenderPass		= m_RenderPass;
		pipelineStateDesc.PipelineLayout	= m_PipelineLayout;

		pipelineStateDesc.DepthStencilState.CompareOp			= ECompareOp::COMPARE_OP_NEVER;
		pipelineStateDesc.DepthStencilState.DepthTestEnable		= false;
		pipelineStateDesc.DepthStencilState.DepthWriteEnable	= false;

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

		pipelineStateDesc.InputLayout =
		{
			{ "POSITION",	0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 0, IM_OFFSETOF(ImDrawVert, pos),	EFormat::FORMAT_R32G32_SFLOAT },
			{ "TEXCOORD",	0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 1, IM_OFFSETOF(ImDrawVert, uv),	EFormat::FORMAT_R32G32_SFLOAT },
			{ "COLOR",		0, sizeof(ImDrawVert), EVertexInputRate::VERTEX_INPUT_PER_VERTEX, 2, IM_OFFSETOF(ImDrawVert, col),	EFormat::FORMAT_R8G8B8A8_UNORM },
		};

		pipelineStateDesc.VertexShader.ShaderGUID	= vertexShader;
		pipelineStateDesc.PixelShader.ShaderGUID	= pixelShader;

		return PipelineStateManager::CreateGraphicsPipelineState(&pipelineStateDesc);
	}
}
