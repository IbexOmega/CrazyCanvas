#include "GUI/Core/GUITexture.h"
#include "GUI/Core/GUIRenderer.h"

#include "Rendering/RenderAPI.h"
#include "Rendering/StagingBufferCache.h"

#include "Rendering/Core/API/GraphicsDevice.h"
#include "Rendering/Core/API/CommandList.h"
#include "Rendering/Core/API/Texture.h"
#include "Rendering/Core/API/TextureView.h"
#include "Rendering/Core/API/Buffer.h"
#include "Rendering/Core/API/GraphicsHelpers.h"

namespace LambdaEngine
{
	GUITexture::GUITexture()
	{
	}

	GUITexture::~GUITexture()
	{
		RenderAPI::EnqueueResourceRelease(m_pTexture);
		RenderAPI::EnqueueResourceRelease(m_pTextureView);

		m_pTexture = nullptr;
		m_pTextureView = nullptr;
	}

	bool GUITexture::Init(CommandList* pCommandList, const GUITextureDesc* pDesc)
	{
		VALIDATE(pDesc != nullptr);

		TextureDesc textureDesc = {};
		textureDesc.DebugName	= pDesc->DebugName + " Texture";
		textureDesc.MemoryType	= EMemoryType::MEMORY_TYPE_GPU;
		textureDesc.Format		= pDesc->Format;
		textureDesc.Type		= ETextureType::TEXTURE_TYPE_2D;
		textureDesc.Flags		= FTextureFlag::TEXTURE_FLAG_RENDER_TARGET | FTextureFlag::TEXTURE_FLAG_SHADER_RESOURCE | FTextureFlag::TEXTURE_FLAG_COPY_SRC | FTextureFlag::TEXTURE_FLAG_COPY_DST;
		textureDesc.Width		= pDesc->Width;
		textureDesc.Height		= pDesc->Height;
		textureDesc.Depth		= 1;
		textureDesc.ArrayCount	= 1;
		textureDesc.Miplevels	= pDesc->MipLevelCount;
		textureDesc.SampleCount = 1;

		m_pTexture = RenderAPI::GetDevice()->CreateTexture(&textureDesc);

		if (m_pTexture == nullptr)
		{
			LOG_ERROR("[GUITexture]: Failed to create Texture");
			return false;
		}

		TextureViewDesc textureViewDesc = {};
		textureViewDesc.DebugName		= pDesc->DebugName + " Texture View";
		textureViewDesc.pTexture		= m_pTexture;
		textureViewDesc.Flags			= FTextureViewFlag::TEXTURE_VIEW_FLAG_RENDER_TARGET | FTextureViewFlag::TEXTURE_VIEW_FLAG_SHADER_RESOURCE;
		textureViewDesc.Format			= textureDesc.Format;
		textureViewDesc.Type			= ETextureViewType::TEXTURE_VIEW_TYPE_2D;
		textureViewDesc.MiplevelCount	= pDesc->MipLevelCount;
		textureViewDesc.ArrayCount		= 1;
		textureViewDesc.Miplevel		= 0;
		textureViewDesc.ArrayIndex		= 0;

		m_pTextureView = RenderAPI::GetDevice()->CreateTextureView(&textureViewDesc);

		if (m_pTextureView == nullptr)
		{
			LOG_ERROR("[GUITexture]: Failed to create Texture View");
			return false;
		}

		PipelineTextureBarrierDesc textureBarrier = { };
		textureBarrier.pTexture				= m_pTexture;
		textureBarrier.TextureFlags			= m_pTexture->GetDesc().Flags;
		textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
		textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
		textureBarrier.Miplevel				= 0;
		textureBarrier.ArrayIndex			= 0;
		textureBarrier.MiplevelCount		= pDesc->MipLevelCount;
		textureBarrier.ArrayCount			= m_pTexture->GetDesc().ArrayCount;
		textureBarrier.SrcMemoryAccessFlags = 0;
		textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
		textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_UNKNOWN;
		textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
		pCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &textureBarrier, 1);

		if (pDesc->ppData != nullptr)
		{
			for (uint32 m = 0; m < pDesc->MipLevelCount; m++)
			{
				uint32 width	= pDesc->Width >> m;
				uint32 height	= pDesc->Height >> m;

				UpdateTexture(pCommandList, m, 0, 0, width, height, pDesc->ppData[m], ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS, ETextureState::TEXTURE_STATE_SHADER_READ_ONLY);
			}
		}

		return true;
	}

	bool GUITexture::Init(LambdaEngine::Texture* pTexture, LambdaEngine::TextureView* pTextureView)
	{
		VALIDATE(pTexture != nullptr);
		VALIDATE(pTextureView != nullptr);

		m_pTexture = pTexture;
		m_pTexture->AddRef();

		m_pTextureView = pTextureView;
		m_pTextureView->AddRef();

		return true;
	}

	uint32_t GUITexture::GetWidth() const
	{
		return m_pTexture->GetDesc().Width;
	}

	uint32_t GUITexture::GetHeight() const
	{
		return m_pTexture->GetDesc().Height;
	}

	bool GUITexture::HasMipMaps() const
	{
		return m_pTexture->GetDesc().Miplevels;
	}

	bool GUITexture::IsInverted() const
	{
		return false;
	}

	void GUITexture::UpdateTexture(CommandList* pCommandList, uint32 mipLevel, uint32 x, uint32 y, uint32 width, uint32 height, const void* pData,
		ECommandQueueType prevCommandQueue, ETextureState prevTextureState)
	{
		uint32 stride = TextureFormatStride(m_pTexture->GetDesc().Format);
		uint64 sizeInBytes = uint64(width * height * stride);

		Buffer* pStagingBuffer = StagingBufferCache::RequestBuffer(sizeInBytes);

		void* pMapped = pStagingBuffer->Map();
		memcpy(pMapped, pData, sizeInBytes);
		pStagingBuffer->Unmap();

		if (prevTextureState != ETextureState::TEXTURE_STATE_COPY_DST)
		{
			PipelineTextureBarrierDesc textureBarrier = { };
			textureBarrier.pTexture				= m_pTexture;
			textureBarrier.TextureFlags			= m_pTexture->GetDesc().Flags;
			textureBarrier.QueueBefore			= prevCommandQueue;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
			textureBarrier.Miplevel				= mipLevel;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayCount			= m_pTexture->GetDesc().ArrayCount;
			textureBarrier.SrcMemoryAccessFlags = 0;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.StateBefore			= prevTextureState;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_COPY_DST;
			pCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_TOP, FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, &textureBarrier, 1);
		}

		CopyTextureBufferDesc copyDesc = {};
		copyDesc.SrcOffset		= 0;
		copyDesc.SrcRowPitch	= 0;
		copyDesc.SrcHeight		= 0;
		copyDesc.OffsetX		= x;
		copyDesc.OffsetY		= y;
		copyDesc.Width			= width;
		copyDesc.Height			= height;
		copyDesc.Depth			= 1;
		copyDesc.Miplevel		= mipLevel;
		copyDesc.MiplevelCount  = 1;
		copyDesc.ArrayIndex		= 0;
		copyDesc.ArrayCount		= m_pTexture->GetDesc().ArrayCount;
		pCommandList->CopyTextureFromBuffer(pStagingBuffer, m_pTexture, copyDesc);

		{
			PipelineTextureBarrierDesc textureBarrier = { };
			textureBarrier.pTexture				= m_pTexture;
			textureBarrier.TextureFlags			= m_pTexture->GetDesc().Flags;
			textureBarrier.QueueBefore			= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
			textureBarrier.QueueAfter			= ECommandQueueType::COMMAND_QUEUE_TYPE_GRAPHICS;
			textureBarrier.Miplevel				= mipLevel;
			textureBarrier.ArrayIndex			= 0;
			textureBarrier.MiplevelCount		= 1;
			textureBarrier.ArrayCount			= m_pTexture->GetDesc().ArrayCount;
			textureBarrier.SrcMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_WRITE;
			textureBarrier.DstMemoryAccessFlags = FMemoryAccessFlag::MEMORY_ACCESS_FLAG_MEMORY_READ;
			textureBarrier.StateBefore			= ETextureState::TEXTURE_STATE_COPY_DST;
			textureBarrier.StateAfter			= ETextureState::TEXTURE_STATE_SHADER_READ_ONLY;
			pCommandList->PipelineTextureBarriers(FPipelineStageFlag::PIPELINE_STAGE_FLAG_COPY, FPipelineStageFlag::PIPELINE_STAGE_FLAG_BOTTOM, &textureBarrier, 1);
		}
	}
}