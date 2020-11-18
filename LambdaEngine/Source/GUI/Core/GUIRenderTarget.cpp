#include "GUI/Core/GUIRenderTarget.h"
#include "GUI/Core/GUIRenderer.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/RenderPass.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"

#include "Rendering/RenderAPI.h"

namespace LambdaEngine
{
	GUIRenderTarget::GUIRenderTarget()
	{
	}

	GUIRenderTarget::~GUIRenderTarget()
	{
		RenderAPI::EnqueueResourceRelease(m_pRenderPass);
		RenderAPI::EnqueueResourceRelease(m_pDepthStencilTexture);
		RenderAPI::EnqueueResourceRelease(m_pDepthStencilTextureView);
		RenderAPI::EnqueueResourceRelease(m_pColorTexture);
		RenderAPI::EnqueueResourceRelease(m_pColorTextureView);
		RenderAPI::EnqueueResourceRelease(m_pResolveTexture);
		RenderAPI::EnqueueResourceRelease(m_pResolveTextureView);

		m_pRenderPass				= nullptr;
		m_pDepthStencilTexture		= nullptr;
		m_pDepthStencilTextureView	= nullptr;
		m_pColorTexture				= nullptr;
		m_pColorTextureView			= nullptr;
		m_pResolveTexture			= nullptr;
		m_pResolveTextureView		= nullptr;
	}

	bool GUIRenderTarget::Init(const GUIRenderTargetDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);
		if (!CreateColorTextures(pDesc))
		{
			LOG_ERROR("[GUIRenderTarget]: Failed to create Color Textures");
			return false;
		}

		if (!CreateDepthStencilTexture(pDesc))
		{
			LOG_ERROR("[GUIRenderTarget]: Failed to create Depth Stencil Texture");
			return false;
		}
		
		if (!CreateRenderPass(pDesc))
		{
			LOG_ERROR("[GUIRenderTarget]: Failed to create RenderPass");
			return false;
		}

		ClearColorDesc* pColorClearColorDesc	= &m_pClearColorDesc[0];
		ClearColorDesc* pResolveClearColorDesc	= &m_pClearColorDesc[1];
		ClearColorDesc* pClearDepthStencilDesc	= &m_pClearColorDesc[2];

		pColorClearColorDesc->Color[0] = 0.0f;
		pColorClearColorDesc->Color[1] = 0.0f;
		pColorClearColorDesc->Color[2] = 0.0f;
		pColorClearColorDesc->Color[3] = 0.0f;

		pResolveClearColorDesc->Color[0] = 0.0f;
		pResolveClearColorDesc->Color[1] = 0.0f;
		pResolveClearColorDesc->Color[2] = 0.0f;
		pResolveClearColorDesc->Color[3] = 0.0f;

		pClearDepthStencilDesc->Depth	= 1.0f;
		pClearDepthStencilDesc->Stencil = 0;

		m_Desc = *pDesc;

		return true;
	}

	Noesis::Texture* GUIRenderTarget::GetTexture()
	{
		return &m_Texture;
	}

	bool GUIRenderTarget::CreateColorTextures(const GUIRenderTargetDesc* pDesc)
	{
		//Color Texture
		{
			TextureDesc textureDesc = {};
			textureDesc.DebugName	= pDesc->DebugName + " Color Texture";
			textureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			textureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
			textureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
			textureDesc.Flags		= TEXTURE_FLAG_RENDER_TARGET;
			textureDesc.Width		= pDesc->Width;
			textureDesc.Height		= pDesc->Height;
			textureDesc.Depth		= 1;
			textureDesc.ArrayCount	= 1;
			textureDesc.Miplevels	= 1;
			textureDesc.SampleCount = FORCED_SAMPLE_COUNT;// pDesc->SampleCount;

			m_pColorTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);
			if (m_pColorTexture == nullptr)
			{
				LOG_ERROR("[GUIRenderTarget]: Failed to create Color Texture");
				return false;
			}

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.DebugName		= pDesc->DebugName + " Color Texture View";
			textureViewDesc.pTexture		= m_pColorTexture;
			textureViewDesc.Flags			= TEXTURE_VIEW_FLAG_RENDER_TARGET;
			textureViewDesc.Format			= textureDesc.Format;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;

			m_pColorTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
			if (m_pColorTextureView == nullptr)
			{
				LOG_ERROR("[GUIRenderTarget]: Failed to create Color Texture View");
				return false;
			}
		}

		//Resolve Texture
		{
			TextureDesc textureDesc = {};
			textureDesc.DebugName	= pDesc->DebugName + " Resolve Texture";
			textureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
			textureDesc.Format		= EFormat::FORMAT_R8G8B8A8_UNORM;
			textureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
			textureDesc.Flags		= TEXTURE_FLAG_RENDER_TARGET | TEXTURE_FLAG_SHADER_RESOURCE;
			textureDesc.Width		= pDesc->Width;
			textureDesc.Height		= pDesc->Height;
			textureDesc.Depth		= 1;
			textureDesc.ArrayCount	= 1;
			textureDesc.Miplevels	= 1;
			textureDesc.SampleCount = 1;

			m_pResolveTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);
			if (m_pResolveTexture == nullptr)
			{
				LOG_ERROR("[GUIRenderTarget]: Failed to create Resolve Texture");
				return false;
			}

			TextureViewDesc textureViewDesc = {};
			textureViewDesc.DebugName		= pDesc->DebugName + " Resolve Texture View";
			textureViewDesc.pTexture		= m_pResolveTexture;
			textureViewDesc.Flags			= TEXTURE_VIEW_FLAG_RENDER_TARGET | TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
			textureViewDesc.Format			= textureDesc.Format;
			textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
			textureViewDesc.MiplevelCount	= 1;
			textureViewDesc.ArrayCount		= 1;
			textureViewDesc.Miplevel		= 0;
			textureViewDesc.ArrayIndex		= 0;

			m_pResolveTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);
			if (m_pResolveTextureView == nullptr)
			{
				LOG_ERROR("[GUIRenderTarget]: Failed to create Resolve Texture View");
				return false;
			}
		}

		//Render Targets
		{
			m_ppRenderTargets[0] = m_pColorTextureView;
			m_ppRenderTargets[1] = m_pResolveTextureView;
		}

		//Init GUITexture
		{
			m_Texture.Init(m_pResolveTexture, m_pResolveTextureView);
		}

		return true;
	}

	bool GUIRenderTarget::CreateDepthStencilTexture(const GUIRenderTargetDesc* pDesc)
	{
		TextureDesc depthStencilTextureDesc = {};
		depthStencilTextureDesc.DebugName	= pDesc->DebugName + " Depth Stencil Texture";
		depthStencilTextureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		depthStencilTextureDesc.Format		= EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthStencilTextureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		depthStencilTextureDesc.Flags		= FTextureFlag::TEXTURE_FLAG_DEPTH_STENCIL;
		depthStencilTextureDesc.Width		= pDesc->Width;
		depthStencilTextureDesc.Height		= pDesc->Height;
		depthStencilTextureDesc.Depth		= 1;
		depthStencilTextureDesc.ArrayCount	= 1;
		depthStencilTextureDesc.Miplevels	= 1;
		depthStencilTextureDesc.SampleCount = FORCED_SAMPLE_COUNT; //pDesc->SampleCount;

		m_pDepthStencilTexture = RenderAPI::GetDevice()->CreateTexture(&depthStencilTextureDesc);

		if (m_pDepthStencilTexture == nullptr)
		{
			LOG_ERROR("[GUIRenderTarget]: Failed to create Depth Stencil Texture");
			return false;
		}

		TextureViewDesc depthStencilTextureViewDesc = {};
		depthStencilTextureViewDesc.DebugName		= pDesc->DebugName + " Depth Stencil Texture View";
		depthStencilTextureViewDesc.pTexture		= m_pDepthStencilTexture;
		depthStencilTextureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_DEPTH_STENCIL;
		depthStencilTextureViewDesc.Format			= depthStencilTextureDesc.Format;
		depthStencilTextureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		depthStencilTextureViewDesc.MiplevelCount	= 1;
		depthStencilTextureViewDesc.ArrayCount		= 1;
		depthStencilTextureViewDesc.Miplevel		= 0;
		depthStencilTextureViewDesc.ArrayIndex		= 0;

		m_pDepthStencilTextureView = RenderAPI::GetDevice()->CreateTextureView(&depthStencilTextureViewDesc);

		if (m_pDepthStencilTextureView == nullptr)
		{
			LOG_ERROR("[GUIRenderTarget]: Failed to create Depth Stencil Texture View");
			return false;
		}

		return true;
	}

	bool GUIRenderTarget::CreateRenderPass(const GUIRenderTargetDesc* pDesc)
	{
		RenderPassAttachmentDesc colorAttachmentDesc = {};
		colorAttachmentDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		colorAttachmentDesc.SampleCount		= FORCED_SAMPLE_COUNT;// pDesc->SampleCount;
		colorAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorAttachmentDesc.InitialState	= ETextureState::TEXTURE_STATE_RENDER_TARGET;
		colorAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_RENDER_TARGET;

		RenderPassAttachmentDesc colorResolveAttachmentDesc = {};
		colorResolveAttachmentDesc.Format			= EFormat::FORMAT_R8G8B8A8_UNORM;
		colorResolveAttachmentDesc.SampleCount		= 1;
		colorResolveAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_LOAD;
		colorResolveAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_STORE;
		colorResolveAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		colorResolveAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_DONT_CARE;
		colorResolveAttachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		colorResolveAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;

		RenderPassAttachmentDesc depthStencilAttachmentDesc = {};
		depthStencilAttachmentDesc.Format			= EFormat::FORMAT_D24_UNORM_S8_UINT;
		depthStencilAttachmentDesc.SampleCount		= FORCED_SAMPLE_COUNT;// pDesc->SampleCount;
		depthStencilAttachmentDesc.LoadOp			= ELoadOp::LOAD_OP_DONT_CARE;
		depthStencilAttachmentDesc.StoreOp			= EStoreOp::STORE_OP_DONT_CARE;
		depthStencilAttachmentDesc.StencilLoadOp	= ELoadOp::LOAD_OP_DONT_CARE;
		depthStencilAttachmentDesc.StencilStoreOp	= EStoreOp::STORE_OP_STORE;
		depthStencilAttachmentDesc.InitialState		= ETextureState::TEXTURE_STATE_DONT_CARE;
		depthStencilAttachmentDesc.FinalState		= ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDesc subpassDesc = {};
		subpassDesc.RenderTargetStates = 
		{ 
			ETextureState::TEXTURE_STATE_RENDER_TARGET, 
			ETextureState::TEXTURE_STATE_DONT_CARE
		};

		subpassDesc.ResolveAttachmentStates = 
		{ 
			ETextureState::TEXTURE_STATE_DONT_CARE,
			ETextureState::TEXTURE_STATE_RENDER_TARGET
		};

		subpassDesc.DepthStencilAttachmentState = ETextureState::TEXTURE_STATE_DEPTH_STENCIL_ATTACHMENT;

		RenderPassSubpassDependencyDesc subpassDependencyDesc = {};
		subpassDependencyDesc.SrcSubpass	= EXTERNAL_SUBPASS;
		subpassDependencyDesc.DstSubpass	= 0;
		subpassDependencyDesc.SrcAccessMask = 0;
		subpassDependencyDesc.DstAccessMask = MEMORY_ACCESS_FLAG_MEMORY_READ | MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		subpassDependencyDesc.SrcStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;
		subpassDependencyDesc.DstStageMask	= FPipelineStageFlag::PIPELINE_STAGE_FLAG_RENDER_TARGET_OUTPUT;

		RenderPassDesc renderPassDesc = {};
		renderPassDesc.DebugName			= pDesc->DebugName + " Render Pass";
		renderPassDesc.Attachments			= { colorAttachmentDesc, colorResolveAttachmentDesc, depthStencilAttachmentDesc };
		renderPassDesc.Subpasses			= { subpassDesc };
		renderPassDesc.SubpassDependencies	= { subpassDependencyDesc };

		m_pRenderPass = RenderAPI::GetDevice()->CreateRenderPass(&renderPassDesc);

		return m_pRenderPass != nullptr;
	}
}