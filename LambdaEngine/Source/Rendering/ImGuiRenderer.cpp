#include "Rendering/ImGuiRenderer.h"
#include "Rendering/RenderSystem.h"

#include "Rendering/Core/API/ICommandAllocator.h"
#include "Rendering/Core/API/IDeviceAllocator.h"
#include "Rendering/Core/API/IGraphicsDevice.h"
#include "Rendering/Core/API/IPipelineLayout.h"
#include "Rendering/Core/API/IDescriptorHeap.h"
#include "Rendering/Core/API/IDescriptorSet.h"
#include "Rendering/Core/API/IPipelineState.h"
#include "Rendering/Core/API/ICommandQueue.h"
#include "Rendering/Core/API/ICommandList.h"
#include "Rendering/Core/API/ITextureView.h"
#include "Rendering/Core/API/IRenderPass.h"
#include "Rendering/Core/API/ITexture.h"
#include "Rendering/Core/API/ISampler.h"
#include "Rendering/Core/API/IShader.h"
#include "Rendering/Core/API/IBuffer.h"

#include "Application/API/IWindow.h"
#include "Application/API/PlatformApplication.h"

#include <imgui.h>

namespace LambdaEngine
{
	// glsl_shader.vert, compiled with:
	// # glslangValidator -V -x -o glsl_shader.vert.u32 glsl_shader.vert
	/*
	#version 450 core
	layout(location = 0) in vec2 aPos;
	layout(location = 1) in vec2 aUV;
	layout(location = 2) in vec4 aColor;
	layout(push_constant) uniform uPushConstant { vec2 uScale; vec2 uTranslate; } pc;
	out gl_PerVertex { vec4 gl_Position; };
	layout(location = 0) out struct { vec4 Color; vec2 UV; } Out;
	void main()
	{
		Out.Color = aColor;
		Out.UV = aUV;
		gl_Position = vec4(aPos * pc.uScale + pc.uTranslate, 0, 1);
	}
	*/
	static uint32_t s_VertSpirv[] =
	{
		0x07230203,0x00010000,0x00080001,0x0000002e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x000a000f,0x00000000,0x00000004,0x6e69616d,0x00000000,0x0000000b,0x0000000f,0x00000015,
		0x0000001b,0x0000001c,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00030005,0x00000009,0x00000000,0x00050006,0x00000009,0x00000000,0x6f6c6f43,
		0x00000072,0x00040006,0x00000009,0x00000001,0x00005655,0x00030005,0x0000000b,0x0074754f,
		0x00040005,0x0000000f,0x6c6f4361,0x0000726f,0x00030005,0x00000015,0x00565561,0x00060005,
		0x00000019,0x505f6c67,0x65567265,0x78657472,0x00000000,0x00060006,0x00000019,0x00000000,
		0x505f6c67,0x7469736f,0x006e6f69,0x00030005,0x0000001b,0x00000000,0x00040005,0x0000001c,
		0x736f5061,0x00000000,0x00060005,0x0000001e,0x73755075,0x6e6f4368,0x6e617473,0x00000074,
		0x00050006,0x0000001e,0x00000000,0x61635375,0x0000656c,0x00060006,0x0000001e,0x00000001,
		0x61725475,0x616c736e,0x00006574,0x00030005,0x00000020,0x00006370,0x00040047,0x0000000b,
		0x0000001e,0x00000000,0x00040047,0x0000000f,0x0000001e,0x00000002,0x00040047,0x00000015,
		0x0000001e,0x00000001,0x00050048,0x00000019,0x00000000,0x0000000b,0x00000000,0x00030047,
		0x00000019,0x00000002,0x00040047,0x0000001c,0x0000001e,0x00000000,0x00050048,0x0000001e,
		0x00000000,0x00000023,0x00000000,0x00050048,0x0000001e,0x00000001,0x00000023,0x00000008,
		0x00030047,0x0000001e,0x00000002,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,
		0x00030016,0x00000006,0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040017,
		0x00000008,0x00000006,0x00000002,0x0004001e,0x00000009,0x00000007,0x00000008,0x00040020,
		0x0000000a,0x00000003,0x00000009,0x0004003b,0x0000000a,0x0000000b,0x00000003,0x00040015,
		0x0000000c,0x00000020,0x00000001,0x0004002b,0x0000000c,0x0000000d,0x00000000,0x00040020,
		0x0000000e,0x00000001,0x00000007,0x0004003b,0x0000000e,0x0000000f,0x00000001,0x00040020,
		0x00000011,0x00000003,0x00000007,0x0004002b,0x0000000c,0x00000013,0x00000001,0x00040020,
		0x00000014,0x00000001,0x00000008,0x0004003b,0x00000014,0x00000015,0x00000001,0x00040020,
		0x00000017,0x00000003,0x00000008,0x0003001e,0x00000019,0x00000007,0x00040020,0x0000001a,
		0x00000003,0x00000019,0x0004003b,0x0000001a,0x0000001b,0x00000003,0x0004003b,0x00000014,
		0x0000001c,0x00000001,0x0004001e,0x0000001e,0x00000008,0x00000008,0x00040020,0x0000001f,
		0x00000009,0x0000001e,0x0004003b,0x0000001f,0x00000020,0x00000009,0x00040020,0x00000021,
		0x00000009,0x00000008,0x0004002b,0x00000006,0x00000028,0x00000000,0x0004002b,0x00000006,
		0x00000029,0x3f800000,0x00050036,0x00000002,0x00000004,0x00000000,0x00000003,0x000200f8,
		0x00000005,0x0004003d,0x00000007,0x00000010,0x0000000f,0x00050041,0x00000011,0x00000012,
		0x0000000b,0x0000000d,0x0003003e,0x00000012,0x00000010,0x0004003d,0x00000008,0x00000016,
		0x00000015,0x00050041,0x00000017,0x00000018,0x0000000b,0x00000013,0x0003003e,0x00000018,
		0x00000016,0x0004003d,0x00000008,0x0000001d,0x0000001c,0x00050041,0x00000021,0x00000022,
		0x00000020,0x0000000d,0x0004003d,0x00000008,0x00000023,0x00000022,0x00050085,0x00000008,
		0x00000024,0x0000001d,0x00000023,0x00050041,0x00000021,0x00000025,0x00000020,0x00000013,
		0x0004003d,0x00000008,0x00000026,0x00000025,0x00050081,0x00000008,0x00000027,0x00000024,
		0x00000026,0x00050051,0x00000006,0x0000002a,0x00000027,0x00000000,0x00050051,0x00000006,
		0x0000002b,0x00000027,0x00000001,0x00070050,0x00000007,0x0000002c,0x0000002a,0x0000002b,
		0x00000028,0x00000029,0x00050041,0x00000011,0x0000002d,0x0000001b,0x0000000d,0x0003003e,
		0x0000002d,0x0000002c,0x000100fd,0x00010038
	};

	// glsl_shader.frag, compiled with:
	// # glslangValidator -V -x -o glsl_shader.frag.u32 glsl_shader.frag
	/*
	#version 450 core
	layout(location = 0) out vec4 fColor;
	layout(set=0, binding=0) uniform sampler2D sTexture;
	layout(location = 0) in struct { vec4 Color; vec2 UV; } In;
	void main()
	{
		fColor = In.Color * texture(sTexture, In.UV.st);
	}
	*/
	static uint32_t s_PixelSpirv[] =
	{
		0x07230203,0x00010000,0x00080001,0x0000001e,0x00000000,0x00020011,0x00000001,0x0006000b,
		0x00000001,0x4c534c47,0x6474732e,0x3035342e,0x00000000,0x0003000e,0x00000000,0x00000001,
		0x0007000f,0x00000004,0x00000004,0x6e69616d,0x00000000,0x00000009,0x0000000d,0x00030010,
		0x00000004,0x00000007,0x00030003,0x00000002,0x000001c2,0x00040005,0x00000004,0x6e69616d,
		0x00000000,0x00040005,0x00000009,0x6c6f4366,0x0000726f,0x00030005,0x0000000b,0x00000000,
		0x00050006,0x0000000b,0x00000000,0x6f6c6f43,0x00000072,0x00040006,0x0000000b,0x00000001,
		0x00005655,0x00030005,0x0000000d,0x00006e49,0x00050005,0x00000016,0x78655473,0x65727574,
		0x00000000,0x00040047,0x00000009,0x0000001e,0x00000000,0x00040047,0x0000000d,0x0000001e,
		0x00000000,0x00040047,0x00000016,0x00000022,0x00000000,0x00040047,0x00000016,0x00000021,
		0x00000000,0x00020013,0x00000002,0x00030021,0x00000003,0x00000002,0x00030016,0x00000006,
		0x00000020,0x00040017,0x00000007,0x00000006,0x00000004,0x00040020,0x00000008,0x00000003,
		0x00000007,0x0004003b,0x00000008,0x00000009,0x00000003,0x00040017,0x0000000a,0x00000006,
		0x00000002,0x0004001e,0x0000000b,0x00000007,0x0000000a,0x00040020,0x0000000c,0x00000001,
		0x0000000b,0x0004003b,0x0000000c,0x0000000d,0x00000001,0x00040015,0x0000000e,0x00000020,
		0x00000001,0x0004002b,0x0000000e,0x0000000f,0x00000000,0x00040020,0x00000010,0x00000001,
		0x00000007,0x00090019,0x00000013,0x00000006,0x00000001,0x00000000,0x00000000,0x00000000,
		0x00000001,0x00000000,0x0003001b,0x00000014,0x00000013,0x00040020,0x00000015,0x00000000,
		0x00000014,0x0004003b,0x00000015,0x00000016,0x00000000,0x0004002b,0x0000000e,0x00000018,
		0x00000001,0x00040020,0x00000019,0x00000001,0x0000000a,0x00050036,0x00000002,0x00000004,
		0x00000000,0x00000003,0x000200f8,0x00000005,0x00050041,0x00000010,0x00000011,0x0000000d,
		0x0000000f,0x0004003d,0x00000007,0x00000012,0x00000011,0x0004003d,0x00000014,0x00000017,
		0x00000016,0x00050041,0x00000019,0x0000001a,0x0000000d,0x00000018,0x0004003d,0x0000000a,
		0x0000001b,0x0000001a,0x00050057,0x00000007,0x0000001c,0x00000017,0x0000001b,0x00050085,
		0x00000007,0x0000001d,0x00000012,0x0000001c,0x0003003e,0x00000009,0x0000001d,0x000100fd,
		0x00010038
	};

	ImGuiRenderer::ImGuiRenderer(const IGraphicsDevice* pGraphicsDevice) :
		m_pGraphicsDevice(pGraphicsDevice)
	{
	}

	ImGuiRenderer::~ImGuiRenderer()
	{
		ImGui::DestroyContext();

		SAFERELEASE(m_pCopyCommandAllocator);
		SAFERELEASE(m_pCopyCommandList);
		SAFERELEASE(m_pAllocator);
		SAFERELEASE(m_pPipelineState);
		SAFERELEASE(m_pPipelineLayout);
		SAFERELEASE(m_pDescriptorHeap);
		SAFERELEASE(m_pDescriptorSet);
		SAFERELEASE(m_pRenderPass);
		SAFERELEASE(m_pVertexShader);
		SAFERELEASE(m_pPixelShader);
		SAFERELEASE(m_pVertexCopyBuffer);
		SAFERELEASE(m_pIndexCopyBuffer);

		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			SAFERELEASE(m_ppVertexBuffers[b]);
			SAFERELEASE(m_ppIndexBuffers[b]);
		}

		SAFEDELETE_ARRAY(m_ppVertexBuffers);
		SAFEDELETE_ARRAY(m_ppIndexBuffers);
		SAFERELEASE(m_pFontTexture);
		SAFERELEASE(m_pFontTextureView);
		SAFERELEASE(m_pSampler);
	}

	bool ImGuiRenderer::Init(const ImGuiRendererDesc* pDesc)
	{
		VALIDATE(pDesc);
		VALIDATE(pDesc->pWindow);

		m_BackBufferCount = pDesc->BackBufferCount;

		uint32 allocatorPageSize = 2 * (4 * pDesc->VertexBufferSize + 4 * pDesc->IndexBufferSize) + MEGA_BYTE(64);

		if (!InitImGui(pDesc->pWindow))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to initialize ImGui");
			return false;
		}

		if (!CreateCopyCommandList())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create copy command list");
			return false;
		}

		if (!CreateAllocator(allocatorPageSize))
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create allocator");
			return false;
		}

		if (!CreateBuffers(pDesc->VertexBufferSize, pDesc->IndexBufferSize))
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

		if (!CreateShaders())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create shaders");
			return false;
		}

		if (!CreateRenderPass())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create RenderPass");
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

		if (!CreatePipelineState())
		{
			LOG_ERROR("[ImGuiRenderer]: Failed to create PipelineState");
			return false;
		}

		m_pDescriptorSet->WriteTextureDescriptors(&m_pFontTextureView, &m_pSampler, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY, 0, 1, EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER);

		PlatformApplication::Get()->AddEventHandler(this);

		return true;
	}

	void ImGuiRenderer::Begin(Timestamp delta, uint32 windowWidth, uint32 windowHeight, float32 scaleX, float32 scaleY)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = float32(delta.AsSeconds());

		io.DisplaySize				= ImVec2((float)windowWidth, (float)windowHeight);
		io.DisplayFramebufferScale	= ImVec2(scaleX, scaleY);

		ImGui::NewFrame();
	}

	void ImGuiRenderer::End()
	{
		ImGui::EndFrame();
		ImGui::Render();
	}

	void ImGuiRenderer::Render(ICommandList* pCommandList, ITextureView* pRenderTarget, uint32 modFrameIndex, uint32 backBufferIndex)
	{
		//Start drawing
		ImGuiIO& io = ImGui::GetIO();
		ImDrawData* pDrawData = ImGui::GetDrawData();

		if (pDrawData == nullptr)
			return;

		if (pDrawData->CmdListsCount == 0)
			return;

		IBuffer* pVertexBuffer	= m_ppVertexBuffers[modFrameIndex];
		IBuffer* pIndexBuffer	= m_ppIndexBuffers[modFrameIndex];

		{
			ImDrawVert* pVertexMapping = nullptr;
			ImDrawIdx* pIndexMapping = nullptr;

			uint32 vertexBufferSize		= 0;
			uint32 indexBufferSize		= 0;
			pVertexMapping				= reinterpret_cast<ImDrawVert*>(m_pVertexCopyBuffer->Map());
			pIndexMapping				= reinterpret_cast<ImDrawIdx*>(m_pIndexCopyBuffer->Map());

			for (int n = 0; n < pDrawData->CmdListsCount; n++)
			{
				const ImDrawList* pDrawList = pDrawData->CmdLists[n];

				memcpy(pVertexMapping + vertexBufferSize,	pDrawList->VtxBuffer.Data, pDrawList->VtxBuffer.Size * sizeof(ImDrawVert));
				memcpy(pIndexMapping + indexBufferSize,		pDrawList->IdxBuffer.Data, pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx));

				vertexBufferSize += pDrawList->VtxBuffer.Size * sizeof(ImDrawVert);
				indexBufferSize += pDrawList->IdxBuffer.Size * sizeof(ImDrawIdx);
			}

			m_pVertexCopyBuffer->Unmap();
			m_pIndexCopyBuffer->Unmap();

			pCommandList->CopyBuffer(m_pVertexCopyBuffer, 0, pVertexBuffer, 0, vertexBufferSize);
			pCommandList->CopyBuffer(m_pIndexCopyBuffer, 0, pIndexBuffer, 0, indexBufferSize);
		}

		uint32 width	= pRenderTarget->GetDesc().pTexture->GetDesc().Width;
		uint32 height	= pRenderTarget->GetDesc().pTexture->GetDesc().Height;

		BeginRenderPassDesc beginRenderPassDesc = {};
		beginRenderPassDesc.pRenderPass			= m_pRenderPass;
		beginRenderPassDesc.ppRenderTargets		= &pRenderTarget;
		beginRenderPassDesc.RenderTargetCount	= 1;
		beginRenderPassDesc.pDepthStencil		= nullptr;
		beginRenderPassDesc.Width				= width;
		beginRenderPassDesc.Height				= height;
		beginRenderPassDesc.Flags				= FRenderPassBeginFlags::RENDER_PASS_BEGIN_FLAG_INLINE;
		beginRenderPassDesc.pClearColors		= nullptr;
		beginRenderPassDesc.ClearColorCount		= 0;
		beginRenderPassDesc.Offset.x			= 0;
		beginRenderPassDesc.Offset.y			= 0;

		pCommandList->BeginRenderPass(&beginRenderPassDesc);
	
		Viewport viewport = {};
		viewport.MinDepth	= 0.0f;
		viewport.MaxDepth	= 1.0f;
		viewport.Width		= (float32)width;
		viewport.Height		= (float32)height;
		viewport.x			= 0.0f;
		viewport.y			= 0.0f;

		pCommandList->SetViewports(&viewport, 0, 1);

		pCommandList->BindGraphicsPipeline(m_pPipelineState);
		pCommandList->BindDescriptorSetGraphics(m_pDescriptorSet, m_pPipelineLayout, 0);

		uint64 offset = 0;
		pCommandList->BindVertexBuffers(&pVertexBuffer, 0, &offset, 1);
		pCommandList->BindIndexBuffer(pIndexBuffer, 0, EIndexType::UINT16);

		// Setup scale and translation:
		// Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
		{
			float32 pScale[2];
			pScale[0] = 2.0f / pDrawData->DisplaySize.x;
			pScale[1] = 2.0f / pDrawData->DisplaySize.y;
			float pTranslate[2];
			pTranslate[0] = -1.0f - pDrawData->DisplayPos.x * pScale[0];
			pTranslate[1] = -1.0f - pDrawData->DisplayPos.y * pScale[1];

			pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, pScale,		2 * sizeof(float32), 0 * sizeof(float32));
			pCommandList->SetConstantRange(m_pPipelineLayout, FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER, pTranslate,	2 * sizeof(float32), 2 * sizeof(float32));
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
					scissorRect.x				= (int32_t)clipRect.x;
					scissorRect.y				= (int32_t)clipRect.y;
					scissorRect.Width			= uint32_t(clipRect.z - clipRect.x);
					scissorRect.Height			= uint32_t(clipRect.w - clipRect.y);

					pCommandList->SetScissorRects(&scissorRect, 0, 1);

					// Draw
					pCommandList->DrawIndexInstanced(pCmd->ElemCount, 1, pCmd->IdxOffset + globalIndexOffset, pCmd->VtxOffset + globalVertexOffset, 0);
				}
			}

			globalIndexOffset	+= pCmdList->IdxBuffer.Size;
			globalVertexOffset	+= pCmdList->VtxBuffer.Size;
		}

		pCommandList->EndRenderPass();
	}

	void ImGuiRenderer::MouseMoved(int32 x, int32 y)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MousePos = ImVec2(float32(x), float32(y));
	}

	void ImGuiRenderer::ButtonPressed(EMouseButton button, uint32 modifierMask)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[button - 1] = true;
	}

	void ImGuiRenderer::ButtonReleased(EMouseButton button)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseDown[button - 1] = false;
	}

	void ImGuiRenderer::MouseScrolled(int32 deltaX, int32 deltaY)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.MouseWheelH	+= (float32)deltaX;
		io.MouseWheel	+= (float32)deltaY;
	}

	void ImGuiRenderer::KeyPressed(EKey key, uint32 modifierMask, bool isRepeat)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[key] = true;
		io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
		io.KeyShift	= io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
		io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
		io.KeySuper	= io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
	}

	void ImGuiRenderer::KeyReleased(EKey key)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeysDown[key] = false;
		io.KeyCtrl	= io.KeysDown[EKey::KEY_LEFT_CONTROL]	|| io.KeysDown[EKey::KEY_RIGHT_CONTROL];
		io.KeyShift	= io.KeysDown[EKey::KEY_LEFT_SHIFT]		|| io.KeysDown[EKey::KEY_RIGHT_SHIFT];
		io.KeyAlt	= io.KeysDown[EKey::KEY_LEFT_ALT]		|| io.KeysDown[EKey::KEY_RIGHT_ALT];
		io.KeySuper	= io.KeysDown[EKey::KEY_LEFT_SUPER]		|| io.KeysDown[EKey::KEY_RIGHT_SUPER];
	}

	void ImGuiRenderer::KeyTyped(uint32 character)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.AddInputCharacter(character);
	}

	bool ImGuiRenderer::InitImGui(IWindow* pWindow)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

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
		io.ImeWindowHandle = pWindow->GetHandle();
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

		return true;
	}

	bool ImGuiRenderer::CreateCopyCommandList()
	{
		m_pCopyCommandAllocator = m_pGraphicsDevice->CreateCommandAllocator("ImGui Copy Command Allocator", ECommandQueueType::COMMAND_QUEUE_GRAPHICS);

		if (m_pCopyCommandAllocator == nullptr)
		{
			return false;
		}

		CommandListDesc commandListDesc = {};
		commandListDesc.pName				= "ImGui Copy Command List";
		commandListDesc.CommandListType		= ECommandListType::COMMAND_LIST_PRIMARY;
		commandListDesc.Flags				= FCommandListFlags::COMMAND_LIST_FLAG_ONE_TIME_SUBMIT;

		m_pCopyCommandList = m_pGraphicsDevice->CreateCommandList(m_pCopyCommandAllocator, &commandListDesc);

		return m_pCopyCommandList != nullptr;
	}

	bool ImGuiRenderer::CreateAllocator(uint32 pageSize)
	{
		DeviceAllocatorDesc allocatorDesc = {};
		allocatorDesc.pName				= "ImGui Allocator";
		allocatorDesc.PageSizeInBytes	= pageSize;

		m_pAllocator = m_pGraphicsDevice->CreateDeviceAllocator(&allocatorDesc);

		return m_pAllocator != nullptr;
	}

	bool ImGuiRenderer::CreateBuffers(uint32 vertexBufferSize, uint32 indexBufferSize)
	{
		BufferDesc vertexCopyBufferDesc = {};
		vertexCopyBufferDesc.pName			= "ImGui Vertex Copy Buffer";
		vertexCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		vertexCopyBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		vertexCopyBufferDesc.SizeInBytes	= vertexBufferSize;

		BufferDesc indexCopyBufferDesc = {};
		indexCopyBufferDesc.pName			= "ImGui Index Copy Buffer";
		indexCopyBufferDesc.MemoryType		= EMemoryType::MEMORY_CPU_VISIBLE;
		indexCopyBufferDesc.Flags			= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		indexCopyBufferDesc.SizeInBytes		= indexBufferSize;

		BufferDesc vertexBufferDesc = {};
		vertexBufferDesc.pName				= "ImGui Vertex Buffer";
		vertexBufferDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		vertexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_VERTEX_BUFFER;
		vertexBufferDesc.SizeInBytes		= vertexBufferSize;

		BufferDesc indexBufferDesc = {};
		indexBufferDesc.pName				= "ImGui Index Buffer";
		indexBufferDesc.MemoryType			= EMemoryType::MEMORY_GPU;
		indexBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_COPY_DST | FBufferFlags::BUFFER_FLAG_INDEX_BUFFER;
		indexBufferDesc.SizeInBytes			= vertexBufferSize;

		m_pVertexCopyBuffer					= m_pGraphicsDevice->CreateBuffer(&vertexCopyBufferDesc,	m_pAllocator);
		m_pIndexCopyBuffer					= m_pGraphicsDevice->CreateBuffer(&indexCopyBufferDesc,		m_pAllocator);

		m_ppVertexBuffers					= DBG_NEW IBuffer*[m_BackBufferCount];
		m_ppIndexBuffers					= DBG_NEW IBuffer*[m_BackBufferCount];
		
		for (uint32 b = 0; b < m_BackBufferCount; b++)
		{
			IBuffer* pVertexBuffer			= m_pGraphicsDevice->CreateBuffer(&vertexBufferDesc,		m_pAllocator);
			IBuffer* pIndexBuffer			= m_pGraphicsDevice->CreateBuffer(&indexBufferDesc,			m_pAllocator);

			if (pVertexBuffer != nullptr && pIndexBuffer != nullptr)
			{
				m_ppVertexBuffers[b]		= pVertexBuffer;
				m_ppIndexBuffers[b]			= pIndexBuffer;
			}
			else
			{
				return false;
			}
		}
		return m_pVertexCopyBuffer != nullptr && m_pIndexCopyBuffer != nullptr;
	}

	bool ImGuiRenderer::CreateTextures()
	{
		ImGuiIO& io = ImGui::GetIO();

		uint8* pPixels		= nullptr;
		int32 width			= 0;
		int32 height		= 0;
		io.Fonts->GetTexDataAsRGBA32(&pPixels, &width, &height);

		int64 textureSize = 4 * width * height;

		BufferDesc fontBufferDesc = {};
		fontBufferDesc.pName				= "ImGui Font Buffer";
		fontBufferDesc.MemoryType			= EMemoryType::MEMORY_CPU_VISIBLE;
		fontBufferDesc.Flags				= FBufferFlags::BUFFER_FLAG_COPY_SRC;
		fontBufferDesc.SizeInBytes			= textureSize;

		IBuffer* pFontBuffer = m_pGraphicsDevice->CreateBuffer(&fontBufferDesc, m_pAllocator);

		if (pFontBuffer == nullptr)
		{
			return false;
		}

		void* pMapped = pFontBuffer->Map();
		memcpy(pMapped, pPixels, textureSize);
		pFontBuffer->Unmap();

		TextureDesc fontTextureDesc = {};
		fontTextureDesc.pName		= "ImGui Font Texture";
		fontTextureDesc.MemoryType  = EMemoryType::MEMORY_GPU;
		fontTextureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureDesc.Type		= ETextureType::TEXTURE_2D;
		fontTextureDesc.Flags		= FTextureFlags::TEXTURE_FLAG_COPY_DST | FTextureFlags::TEXTURE_FLAG_SHADER_RESOURCE;
		fontTextureDesc.Width		= width;
		fontTextureDesc.Height		= height;
		fontTextureDesc.Depth		= 1;
		fontTextureDesc.ArrayCount	= 1;
		fontTextureDesc.Miplevels	= 1;
		fontTextureDesc.SampleCount = 1;

		m_pFontTexture = m_pGraphicsDevice->CreateTexture(&fontTextureDesc, m_pAllocator);

		if (m_pFontTexture == nullptr)
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

		m_pCopyCommandAllocator->Reset();
		m_pCopyCommandList->Begin(nullptr);

		PipelineTextureBarrierDesc transitionToCopyDstBarrier = {};
		transitionToCopyDstBarrier.pTexture					= m_pFontTexture;
		transitionToCopyDstBarrier.StateBefore				= ETextureState::TEXTURE_STATE_UNKNOWN;
		transitionToCopyDstBarrier.StateAfter				= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToCopyDstBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToCopyDstBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToCopyDstBarrier.SrcMemoryAccessFlags		= 0;
		transitionToCopyDstBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToCopyDstBarrier.TextureFlags				= fontTextureDesc.Flags;
		transitionToCopyDstBarrier.Miplevel					= 0;
		transitionToCopyDstBarrier.MiplevelCount			= fontTextureDesc.Miplevels;
		transitionToCopyDstBarrier.ArrayIndex				= 0;
		transitionToCopyDstBarrier.ArrayCount				= fontTextureDesc.ArrayCount;

		m_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, &transitionToCopyDstBarrier, 1);
		m_pCopyCommandList->CopyTextureFromBuffer(pFontBuffer, m_pFontTexture, copyDesc);
		
		PipelineTextureBarrierDesc transitionToShaderReadBarrier = {};
		transitionToShaderReadBarrier.pTexture					= m_pFontTexture;
		transitionToShaderReadBarrier.StateBefore				= ETextureState::TEXTURE_STATE_COPY_DST;
		transitionToShaderReadBarrier.StateAfter				= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		transitionToShaderReadBarrier.QueueBefore				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToShaderReadBarrier.QueueAfter				= ECommandQueueType::COMMAND_QUEUE_NONE;
		transitionToShaderReadBarrier.SrcMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		transitionToShaderReadBarrier.DstMemoryAccessFlags		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ;
		transitionToShaderReadBarrier.TextureFlags				= fontTextureDesc.Flags;
		transitionToShaderReadBarrier.Miplevel					= 0;
		transitionToShaderReadBarrier.MiplevelCount				= fontTextureDesc.Miplevels;
		transitionToShaderReadBarrier.ArrayIndex				= 0;
		transitionToShaderReadBarrier.ArrayCount				= fontTextureDesc.ArrayCount;

		m_pCopyCommandList->PipelineTextureBarriers(FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlags::PIPELINE_STAGE_FLAG_BOTTOM, &transitionToShaderReadBarrier, 1);

		m_pCopyCommandList->End();

		RenderSystem::GetGraphicsQueue()->ExecuteCommandLists(&m_pCopyCommandList, 1, FPipelineStageFlags::PIPELINE_STAGE_FLAG_COPY, nullptr, 0, nullptr, 0);
		RenderSystem::GetGraphicsQueue()->Flush();

		SAFEDELETE(pFontBuffer);

		TextureViewDesc fontTextureViewDesc = {};
		fontTextureViewDesc.pName			= "ImGui Font Texture View";
		fontTextureViewDesc.pTexture		= m_pFontTexture;
		fontTextureViewDesc.Flags			= FTextureViewFlags::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		fontTextureViewDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		fontTextureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_2D;
		fontTextureViewDesc.MiplevelCount	= 1;
		fontTextureViewDesc.ArrayCount		= 1;
		fontTextureViewDesc.Miplevel		= 0;
		fontTextureViewDesc.ArrayIndex		= 0;

		m_pFontTextureView = m_pGraphicsDevice->CreateTextureView(&fontTextureViewDesc);

		return m_pFontTextureView != nullptr;
	}

	bool ImGuiRenderer::CreateSamplers()
	{
		SamplerDesc samplerDesc = {};
		samplerDesc.pName				= "ImGui Sampler";
		samplerDesc.MinFilter			= EFilter::LINEAR;
		samplerDesc.MagFilter			= EFilter::LINEAR;
		samplerDesc.MipmapMode			= EMipmapMode::LINEAR;
		samplerDesc.AddressModeU		= EAddressMode::MIRRORED_REPEAT;
		samplerDesc.AddressModeV		= samplerDesc.AddressModeU;
		samplerDesc.AddressModeW		= samplerDesc.AddressModeU;
		samplerDesc.MipLODBias			= 0.0f; 
		samplerDesc.AnisotropyEnabled	= false;
		samplerDesc.MaxAnisotropy		= 16;
		samplerDesc.MinLOD				= 0.0f;
		samplerDesc.MaxLOD				= 1.0f;

		m_pSampler = m_pGraphicsDevice->CreateSampler(&samplerDesc);

		return m_pSampler != nullptr;
	}

	bool ImGuiRenderer::CreateShaders()
	{
		ShaderDesc vertexShaderDesc = {};
		vertexShaderDesc.pName			= "ImGui Vertex Shader";
		vertexShaderDesc.pSource		= reinterpret_cast<const char*>(s_VertSpirv);
		vertexShaderDesc.SourceSize		= sizeof(s_VertSpirv);
		vertexShaderDesc.pEntryPoint	= "main";
		vertexShaderDesc.Stage			= FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
		vertexShaderDesc.Lang			= EShaderLang::SPIRV;

		ShaderDesc pixelShaderDesc = {};
		pixelShaderDesc.pName			= "ImGui Pixel Shader";
		pixelShaderDesc.pSource			= reinterpret_cast<const char*>(s_PixelSpirv);
		pixelShaderDesc.SourceSize		= sizeof(s_PixelSpirv);
		pixelShaderDesc.pEntryPoint		= "main";
		pixelShaderDesc.Stage			= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		pixelShaderDesc.Lang			= EShaderLang::SPIRV;

		m_pVertexShader = m_pGraphicsDevice->CreateShader(&vertexShaderDesc);
		m_pPixelShader = m_pGraphicsDevice->CreateShader(&pixelShaderDesc);

		return m_pVertexShader != nullptr && m_pPixelShader != nullptr;
	}

	bool ImGuiRenderer::CreateRenderPass()
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_B8G8R8A8_UNORM;
		colorAttachmentDesc.SampleCount		= 1;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::DONT_CARE;
		colorAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_PRESENT;
		colorAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_PRESENT;

		ETextureState pTextureState[1] = { ETextureState::TEXTURE_STATE_RENDER_TARGET };

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.pInputAttachmentStates		= nullptr;
		subpassDesc.InputAttachmentCount		= 0;
		subpassDesc.pRenderTargetStates			= pTextureState;
		subpassDesc.pResolveAttachmentStates	= nullptr;
		subpassDesc.RenderTargetCount			= 1;
		subpassDesc.DepthStencilAttachmentState	= ETextureState::TEXTURE_STATE_DONT_CARE;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass		= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass		= 0;
		subpassDependencyDesc.SrcAccessMask		= 0;
		subpassDependencyDesc.DstAccessMask		= FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_READ | FMemoryAccessFlags::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask		= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask		= FPipelineStageFlags::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.pName					= "ImGui Render Pass";
		renderPassDesc.pAttachments				= &colorAttachmentDesc;
		renderPassDesc.AttachmentCount			= 1;
		renderPassDesc.pSubpasses				= &subpassDesc;
		renderPassDesc.SubpassCount				= 1;
		renderPassDesc.pSubpassDependencies		= &subpassDependencyDesc;
		renderPassDesc.SubpassDependencyCount	= 1;

		m_pRenderPass = m_pGraphicsDevice->CreateRenderPass(&renderPassDesc);

		return m_pRenderPass != nullptr;
	}

	bool ImGuiRenderer::CreatePipelineLayout()
	{
		DescriptorBindingDesc descriptorBindingDesc = {};
		descriptorBindingDesc.DescriptorType		= EDescriptorType::DESCRIPTOR_SHADER_RESOURCE_COMBINED_SAMPLER;
		descriptorBindingDesc.DescriptorCount		= 1;
		descriptorBindingDesc.Binding				= 0;
		descriptorBindingDesc.ShaderStageMask		= FShaderStageFlags::SHADER_STAGE_FLAG_PIXEL_SHADER;
		descriptorBindingDesc.ppImmutableSamplers	= nullptr;

		DescriptorSetLayoutDesc descriptorSetLayoutDesc = {};
		descriptorSetLayoutDesc.pDescriptorBindings		= &descriptorBindingDesc;
		descriptorSetLayoutDesc.DescriptorBindingCount	= 1;

		ConstantRangeDesc constantRangeDesc = {};
		constantRangeDesc.ShaderStageFlags		= FShaderStageFlags::SHADER_STAGE_FLAG_VERTEX_SHADER;
		constantRangeDesc.SizeInBytes			= 4 * sizeof(float32);
		constantRangeDesc.OffsetInBytes			= 0;

		PipelineLayoutDesc pipelineLayoutDesc = {};
		pipelineLayoutDesc.pName						= "ImGui Pipeline Layout";
		pipelineLayoutDesc.pDescriptorSetLayouts		= &descriptorSetLayoutDesc;
		pipelineLayoutDesc.DescriptorSetLayoutCount		= 1;
		pipelineLayoutDesc.pConstantRanges				= &constantRangeDesc;
		pipelineLayoutDesc.ConstantRangeCount			= 1;

		m_pPipelineLayout = m_pGraphicsDevice->CreatePipelineLayout(&pipelineLayoutDesc);

		return m_pPipelineLayout != nullptr;
	}

	bool ImGuiRenderer::CreateDescriptorSet()
	{
		DescriptorCountDesc descriptorCountDesc = {};
		descriptorCountDesc.DescriptorSetCount						= 64;
		descriptorCountDesc.SamplerDescriptorCount					= 1;
		descriptorCountDesc.TextureDescriptorCount					= 1;
		descriptorCountDesc.TextureCombinedSamplerDescriptorCount	= 64;
		descriptorCountDesc.ConstantBufferDescriptorCount			= 1;
		descriptorCountDesc.UnorderedAccessBufferDescriptorCount	= 1;
		descriptorCountDesc.UnorderedAccessTextureDescriptorCount	= 1;
		descriptorCountDesc.AccelerationStructureDescriptorCount	= 1;

		DescriptorHeapDesc descriptorHeapDesc = {};
		descriptorHeapDesc.pName				= "ImGui Descriptor Heap";
		descriptorHeapDesc.DescriptorCount		= descriptorCountDesc;

		m_pDescriptorHeap = m_pGraphicsDevice->CreateDescriptorHeap(&descriptorHeapDesc);

		if (m_pDescriptorHeap == nullptr)
			return false;

		m_pDescriptorSet = m_pGraphicsDevice->CreateDescriptorSet("ImGui Descriptor Set", m_pPipelineLayout, 0, m_pDescriptorHeap);

		return m_pDescriptorSet != nullptr;
	}

	bool ImGuiRenderer::CreatePipelineState()
	{
		VertexInputAttributeDesc pVertexAttributeDesc[3] = {};
		pVertexAttributeDesc[0].Location	= 0;
		pVertexAttributeDesc[0].Offset		= IM_OFFSETOF(ImDrawVert, pos);
		pVertexAttributeDesc[0].Format		= EFormat::FORMAT_R32G32_SFLOAT;
		pVertexAttributeDesc[1].Location	= 1;
		pVertexAttributeDesc[1].Offset		= IM_OFFSETOF(ImDrawVert, uv);
		pVertexAttributeDesc[1].Format		= EFormat::FORMAT_R32G32_SFLOAT;
		pVertexAttributeDesc[2].Location	= 2;
		pVertexAttributeDesc[2].Offset		= IM_OFFSETOF(ImDrawVert, col);
		pVertexAttributeDesc[2].Format		= EFormat::FORMAT_R8G8B8A8_UNORM;

		VertexInputBindingDesc vertexInputBindingDesc = {};
		vertexInputBindingDesc.Binding			= 0;
		vertexInputBindingDesc.Stride			= sizeof(ImDrawVert);
		vertexInputBindingDesc.InputRate		= EVertexInputRate::PER_VERTEX;
		vertexInputBindingDesc.pAttributes		= pVertexAttributeDesc;
		vertexInputBindingDesc.AttributeCount	= 3;

		BlendAttachmentState blendAttachmentState = {};
		blendAttachmentState.BlendEnabled			= true;
		blendAttachmentState.ColorComponentsMask	= COLOR_COMPONENT_FLAG_R | COLOR_COMPONENT_FLAG_G | COLOR_COMPONENT_FLAG_B | COLOR_COMPONENT_FLAG_A;

		GraphicsPipelineStateDesc pipelineStateDesc = {};
		pipelineStateDesc.pName							= "ImGui Pipeline State";
		pipelineStateDesc.pRenderPass					= m_pRenderPass;
		pipelineStateDesc.pPipelineLayout				= m_pPipelineLayout;
		pipelineStateDesc.pVertexInputBindings			= &vertexInputBindingDesc;
		pipelineStateDesc.VertexInputBindingCount		= 1;
		pipelineStateDesc.pBlendAttachmentStates[0]		= blendAttachmentState;
		pipelineStateDesc.BlendAttachmentStateCount		= 1;
		pipelineStateDesc.pVertexShader					= m_pVertexShader;
		pipelineStateDesc.pPixelShader					= m_pPixelShader;

		m_pPipelineState = m_pGraphicsDevice->CreateGraphicsPipelineState(&pipelineStateDesc);

		return m_pPipelineState != nullptr;
	}
}